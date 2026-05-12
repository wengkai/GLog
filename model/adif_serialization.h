#ifndef ADIF_SERIALIZATION_H
#define ADIF_SERIALIZATION_H

#include <string>
#include <vector>

class QTextStream;

#include "record.h"

#include "app_export.h"

namespace AdifSerialization {

GLOGKIT_API void toCsv(QTextStream &stream, const std::vector<GRecord> &p_records,
                       const std::vector<std::string> &p_rheaders);

GLOGKIT_API void toAdif(QTextStream &stream, const std::vector<GRecord> &p_records);

} // namespace AdifSerialization

#endif
