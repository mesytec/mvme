#include "mvme_stream_server.h"

#include <cassert>
#include <mesytec-mvlc/util/logging.h>
#include <mesytec-mvlc/stream_server_interface.h>
#include <mesytec-mvlc/stream_server_asio.h>
#include <boost/endian/conversion.hpp>

#include "mvme_workspace.h"
#include "util/expand_env_vars.h"
#include "util/qt_str.h"

namespace mesytec::mvme
{

struct MvmeStreamServer::Private
{
    bool enabled_ = false;
    bool sendRawFormat_ = false;
    std::vector<std::string> listenUris_;
    std::shared_ptr<spdlog::logger> logger_;
    StreamConsumerBase::Logger mvmeLogger_;
    std::unique_ptr<mvlc::IStreamServer> server_;
    std::mutex mutex_; // protects everything! :)
    size_t startupResult_ = false;
    std::vector<u32> localBuffer_;
};

const std::vector<std::string> MvmeStreamServer::DefaultListenUris = {
    "tcp4://*:42333",
#ifndef WIN32
    "ipc://${XDG_RUNTIME_DIR}/mvme_stream_server.sock",
#endif
};

MvmeStreamServer::MvmeStreamServer()
    : IStreamBufferConsumer()
    , d(std::make_unique<Private>())
{
    d->logger_ = mvlc::get_logger("mvme_stream_server");
    d->server_ = std::make_unique<mvlc::StreamServerAsio>();
}

MvmeStreamServer::~MvmeStreamServer()
{
}

void MvmeStreamServer::startup()
{

    std::unique_lock<std::mutex> lock(d->mutex_);
    if (d->enabled_ && !d->listenUris_.empty())
    {
        d->startupResult_ = 0;
        for (const auto &uri: d->listenUris_)
        {
            if (!d->server_->listen(uri))
            {
                d->logger_->error("MvmeStreamServer: Failed to listen on URI: {}", uri);
                ++d->startupResult_;
            }
            else
            {
                d->logger_->info("MvmeStreamServer: Listening on URI: {}", uri);
            }
        }
    }
}

void MvmeStreamServer::shutdown()
{
    std::unique_lock<std::mutex> lock(d->mutex_);
    d->server_->stop();
    d->startupResult_ = 0;
    d->logger_->info("MvmeStreamServer: stopped");
}

void MvmeStreamServer::beginRun(const RunInfo &runInfo, const VMEConfig *vmeConfig,
                                   const analysis::Analysis *analysis)
{
    Q_UNUSED(runInfo);
    Q_UNUSED(vmeConfig);
    Q_UNUSED(analysis);
}

void MvmeStreamServer::endRun(const DAQStats &stats, const std::exception *e)
{
    Q_UNUSED(stats);
    Q_UNUSED(e);
}

void MvmeStreamServer::processBuffer(s32 bufferType, u32 bufferNumber, const u32 *buffer,
                                     size_t bufferSize)
{
    std::unique_lock<std::mutex> lock(d->mutex_);
    Q_UNUSED(bufferType);
    assert(bufferSize <= std::numeric_limits<u32>::max());

    size_t wordsNeeded = d->sendRawFormat_ ? bufferSize : (2 + bufferSize);
    d->localBuffer_.resize(wordsNeeded);

    if (!d->sendRawFormat_)
    {
        d->localBuffer_[0] = boost::endian::native_to_little(bufferNumber);
        d->localBuffer_[1] = boost::endian::native_to_little(static_cast<u32>(bufferSize));
    }

    std::transform(buffer, buffer + bufferSize,
                   d->localBuffer_.data() + (d->sendRawFormat_ ? 0 : 2),
                   [](u32 val) { return boost::endian::native_to_little(val); });

    d->server_->sendToAllClients(reinterpret_cast<const u8 *>(d->localBuffer_.data()),
                                 d->localBuffer_.size() * sizeof(u32));
}

void MvmeStreamServer::setLogger(StreamConsumerBase::Logger logger)
{
    d->mvmeLogger_ = logger;
}

StreamConsumerBase::Logger &MvmeStreamServer::getLogger()
{
    return d->mvmeLogger_;
}

void MvmeStreamServer::reloadConfiguration()
{
    auto settings = make_workspace_settings();

    d->enabled_ = settings.value(QSL("StreamServer/Enabled")).toBool();
    d->sendRawFormat_ = settings.value(QSL("StreamServer/SendRawFormat")).toBool();

    d->logger_->trace("MvmeStreamServer::reloadConfiguration(): StreamServer/Enabled={}",
                 d->enabled_);

    if (!d->enabled_ && d->server_->isListening())
    {
        logMessage(QSL("StreamServer is disabled, shutting down"));
        shutdown();
        return;
    }

    d->listenUris_.clear();

    for (const auto &qtUri: settings.value(QSL("StreamServer/ListenUris")).toStringList())
    {
        auto expanded = util::expand_env_vars(qtUri.toStdString());
        d->listenUris_.emplace_back(expanded);
    }

    shutdown();

    if (!d->listenUris_.empty() && d->enabled_)
    {
        startup();
    }
}

} // namespace mesytec::mvme
