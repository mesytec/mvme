#include "mvme_stream_server.h"

#include <cassert>
#include <mesytec-mvlc/util/logging.h>
#include <mesytec-mvlc/stream_server_interface.h>
#include <mesytec-mvlc/stream_server_asio.h>

#include "mvme_workspace.h"
#include "util/expand_env_vars.h"
#include "util/qt_str.h"

namespace mesytec::mvme
{

struct MvmeStreamServer::Private
{
    bool enabled_ = false;
    std::vector<std::string> listenUris_;
    std::shared_ptr<spdlog::logger> logger_;
    StreamConsumerBase::Logger mvmeLogger_;
    std::unique_ptr<mvlc::IStreamServer> server_;
    std::mutex mutex_; // protects everything! :)
    size_t startupResult_ = false;
};

const std::vector<std::string> MvmeStreamServer::DefaultListenUris = {
    "tcp4://*:42333",
#ifndef WIN32
    "ipc://${XDG_RUNTIME_DIR}/mvme_stream_server.socket",
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
        d->startupResult_ = d->server_->listen(d->listenUris_);
}

void MvmeStreamServer::shutdown()
{
    std::unique_lock<std::mutex> lock(d->mutex_);
    d->server_->stop();
    d->startupResult_ = 0;
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

    u32 bufferSizeU32 = static_cast<u32>(bufferSize);

    std::array<mvlc::IStreamServer::IOV, 3> iov{};

    // format is: bufferNumber: u32, bufferSize: u32, buffer: u32[]
    iov[0].buf = &bufferNumber;
    iov[0].len = sizeof(bufferNumber);

    iov[1].buf = &bufferSizeU32;
    iov[1].len = sizeof(bufferSizeU32);

    iov[2].buf = buffer;
    iov[2].len = bufferSize * sizeof(u32);

    // TODO: do byteswapping here! This is the last place where we know the data is u32, not u8.

    d->server_->sendToAllClients(iov.data(), iov.size());
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

        if (d->startupResult_ != d->listenUris_.size())
            logMessage(QSL("StreamServer failed to listen on at least one URI! See the console log for details."));
        else
            logMessage(fmt::format("StreamServer listening on: {}", fmt::join(d->listenUris_, ", ")).c_str());
    }
}

} // namespace mesytec::mvme
