#ifndef ADIF_SERIALIZATION_H
#define ADIF_SERIALIZATION_H

#include <string>
#include <vector>

class QTextStream;

#include "record.h"

namespace AdifSerialization {

void toCsv(QTextStream &stream, const std::vector<GRecord> &p_records,
           const std::vector<std::string> &p_rheaders);

void toAdif(QTextStream &stream, const std::vector<GRecord> &p_records);

} // namespace AdifSerialization

#endif
