#ifndef AWARD_ENTITY_COUNT_REPORT_H
#define AWARD_ENTITY_COUNT_REPORT_H

#include <QString>
#include <vector>

#include "record.h"

class IAwardPluginsManager;

/** DXCC/WAC/CQZ counts plus enabled award plugin summaries for the given QSO rows. */
class AwardEntityCountReport {
  public:
    explicit AwardEntityCountReport(IAwardPluginsManager *broker = nullptr);

    QString format(const std::vector<GRecord> &records) const;

  private:
    IAwardPluginsManager *m_broker;
};

#endif
