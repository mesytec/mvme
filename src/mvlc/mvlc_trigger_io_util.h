/* mvme - Mesytec VME Data Acquisition
 *
 * Copyright (C) 2016-2023 mesytec GmbH & Co. KG <info@mesytec.com>
 *
 * Author: Florian Lüke <f.lueke@mesytec.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#ifndef __MVME_MVLC_TRIGGER_IO_UTIL_H__
#define __MVME_MVLC_TRIGGER_IO_UTIL_H__

#include "libmvme_export.h"
#include <QTextStream>
#include <mesytec-mvlc/mesytec-mvlc.h>

namespace mesytec::mvme_mvlc::trigger_io
{

static const QString MetaTagMVLCTriggerIO = "mvlc_trigger_io";

LIBMVME_EXPORT QTextStream &
print_front_panel_io_table(QTextStream &out, const mesytec::mvlc::trigger_io::TriggerIO &ioCfg);

} // namespace mesytec::mvme_mvlc::trigger_io

#endif /* __MVME_MVLC_TRIGGER_IO_UTIL_H__ */
