#ifndef IGRECORD_H
#define IGRECORD_H

#include <cstdint>

class IGRecord {
  public:
    enum class Result : uint8_t {
        NoError = 0,
        InvalidInput,
        InvalidResultLenOutput,
        NotFound,
        Overflow,
        InternalError = 255,
    };
};

extern "C" {
/**
 * @brief Retrieves the value associated with a specific field name.
 * This method implements a robust buffer-copy pattern to safely export data across
 * API/ABI boundaries without transferring ownership of internal memory.
 *
 * @param[in]  field          Pointer to the LOWER field name string (non-null).
 * @param[in]  field_len      Length of the field name in bytes.
 * @param[out] result_buf     Pointer to the destination buffer. If nullptr, the method can be used
 * to query the required size.
 * @param[out] result_len     Pointer to a variable that receives the actual size of the data. Must
 * not be nullptr.
 * @param[in]  max_result_len The maximum capacity of result_buf.
 * @return Result::NoError             Success. Data copied to result_buf.
 * @return Result::Overflow            Buffer is null or too small. result_len updated with the
 * required size.
 * @return Result::NotFound            Field does not exist in the record.
 * @return Result::InvalidInput        QSO/Field pointer is null or field_len is 0.
 * @return Result::InvalidResultLenOutput result_len pointer is null.
 * @note This method is marked noexcept and will not throw. It is the caller's responsibility to
 * ensure result_buf is large enough based on a prior call or pre-determined constraints.
 */
typedef IGRecord::Result (*IGRecordGetValueByField)(const IGRecord *QSO, const char *field,
                                                    uint64_t field_len, char *result_buf,
                                                    uint64_t *result_len, uint64_t max_result_len);
};

#endif