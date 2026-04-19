// adif_stress_generator.hpp
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace adif_stress {

/**
 * @brief ADIF 压测数据集生成器（header-only）
 *
 * 支持生成各类畸形、超大、逻辑错误以及大批量 ADIF 数据，用于测试解析器健壮性。
 *
 * 使用方法示例：
 * @code
 *   #include "adif_stress_generator.hpp"
 *
 *   // 直接生成并保存到文件
 *   adif_stress::Generator::saveToFile("large_field.adi",
 *                                      adif_stress::Generator::makeLargeField(512));
 *
 *   // 生成百万级 QSO 记录并写入文件（流式写入，避免内存爆炸）
 *   adif_stress::Generator::writeVolumeFile("million_qsos.adi", 1'000'000);
 *
 *   // 获取字符串内容用于内存测试
 *   std::string data = adif_stress::Generator::makeNestedTag();
 * @endcode
 */
class Generator {
  public:
    // ----------------------------- 1. 结构化压力测试 -----------------------------

    /** 超大字段长度：<CALL:xxx> 后填充指定长度的 'X' */
    static std::string makeLargeField(size_t length) {
        std::ostringstream oss;
        writeHeader(oss);
        oss << "<CALL:" << length << '>' << std::string(length, 'X');
        oss << "<EOR>\n";
        return oss.str();
    }

    /** 数据区内包含伪标签：<NOTES> 中包含 "<TAG>" */
    static std::string makeNestedTag() {
        std::ostringstream oss;
        writeHeader(oss);
        oss << "<NOTES:15>Contains <TAG>\n"
            << "<EOR>\n";
        return oss.str();
    }

    /** 未知标签：非标准字段 <MY_NEW_FIELD> */
    static std::string makeUnknownTag() {
        std::ostringstream oss;
        writeHeader(oss);
        oss << "<CALL:4>W1AW\n"
            << "<MY_NEW_FIELD:5>HELLO\n"
            << "<EOR>\n";
        return oss.str();
    }

    /** 无数据标签：紧跟 <EOR> 或 <EOM> */
    static std::string makeEmptyTag() {
        std::ostringstream oss;
        writeHeader(oss);
        oss << "<CALL:4>W1AW\n"
            << "<NOTES:0>\n" // 零长度字段
            << "<EOR>\n"
            << "<EOM>\n"; // 单独的结束标签
        return oss.str();
    }

    /** 乱序标签：QSO_DATE 出现在 CALL 之后，且混入多个字段 */
    static std::string makeOutOfOrderTags() {
        std::ostringstream oss;
        writeHeader(oss);
        oss << "<BAND:3>20M\n"
            << "<CALL:4>W1AW\n"
            << "<QSO_DATE:8>20240315\n"
            << "<MODE:3>SSB\n"
            << "<EOR>\n";
        return oss.str();
    }

    // ----------------------------- 2. 字符编码与边界测试 -----------------------------

    /** 混合 UTF-8 字符（如 ° 符号）以及非打印字符 */
    static std::string makeEncodingTest() {
        std::ostringstream oss;
        writeHeader(oss);
        // 经纬度符号 ° 占 2 字节 (UTF-8: C2 B0)
        oss << "<CALL:6>W1AW/Ø\n"             // 包含斜杠和特殊字符
            << "<GRIDSQUARE:6>FN42°A\n"       // 度符号
            << "<COMMENT:13>Café con leche\n" // é 字符
            << "<NOTES:10>Line1\r\nLine2\nLine3\r\n"
            << "<EOR>\n";
        return oss.str();
    }

    /** 大小写混合的标签名 */
    static std::string makeCaseInsensitiveTags() {
        std::ostringstream oss;
        writeHeader(oss);
        oss << "<Call:4>W1AW\n"
            << "<band:3>20M\n"
            << "<MODE:3>SSB\n"
            << "<EOR>\n";
        return oss.str();
    }

    /** 包含 XML/HTML 敏感字符：& " ' < > */
    static std::string makeSpecialChars() {
        std::ostringstream oss;
        writeHeader(oss);
        oss << "<NOTES:25>AT&T \"radio\" <ham> 'test'\n"
            << "<EOR>\n";
        return oss.str();
    }

    // ----------------------------- 3. 数据逻辑压力测试 -----------------------------

    /** 非法日期/时间 */
    static std::string makeInvalidDateTime() {
        std::ostringstream oss;
        writeHeader(oss);
        oss << "<CALL:4>W1AW\n"
            << "<QSO_DATE:8>20261332\n" // 不存在的月份日期
            << "<TIME_ON:4>2560\n"      // 不存在的小时
            << "<TIME_OFF:4>99:99\n"    // 格式错误
            << "<EOR>\n";
        return oss.str();
    }

    /** 模式违规：非标准枚举值 */
    static std::string makeInvalidMode() {
        std::ostringstream oss;
        writeHeader(oss);
        oss << "<CALL:4>W1AW\n"
            << "<MODE:15>NO_SUCH_MODE\n"
            << "<SUBMODE:6>FT999\n"
            << "<EOR>\n";
        return oss.str();
    }

    /** 频率越界：负值 / 天文数字 */
    static std::string makeInvalidFrequency() {
        std::ostringstream oss;
        writeHeader(oss);
        oss << "<CALL:4>W1AW\n"
            << "<FREQ:4>-7.0\n"
            << "<FREQ_RX:9>999999.99\n"
            << "<EOR>\n";
        return oss.str();
    }

    // ----------------------------- 4. 规模化压测 -----------------------------

    /**
     * @brief 生成指定数量的 QSO 记录（内存字符串版本，适用于中小规模）
     * @param count 记录数
     * @return ADIF 格式字符串
     * @note 对于 >10万 记录建议使用 writeVolumeFile() 直接写文件
     */
    static std::string makeVolume(size_t count) {
        std::ostringstream oss;
        writeHeader(oss);
        for (size_t i = 0; i < count; ++i) {
            appendRandomQSO(oss);
            oss << "<EOR>\n";
        }
        return oss.str();
    }

    /**
     * @brief 流式写入大规模 ADIF 文件（推荐用于百万级记录）
     * @param filename 输出文件名
     * @param count 记录数
     * @param seed 随机种子（默认固定，保证可重复）
     */
    static void writeVolumeFile(const std::string &filename, size_t count, uint32_t seed = 42) {
        std::ofstream out(filename, std::ios::binary);
        if (!out) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        writeHeader(out);
        std::mt19937 rng(seed);
        for (size_t i = 0; i < count; ++i) {
            appendRandomQSO(out, rng);
            out << "<EOR>\n";
            // 每 10k 条刷新一次，避免缓冲区过大
            if (i % 10000 == 0) out.flush();
        }
    }

    // ----------------------------- 5. 异常终止测试 -----------------------------

    /** 截断文件：在标签中间结束 */
    static std::string makeTruncatedInTag() {
        std::ostringstream oss;
        writeHeader(oss);
        oss << "<CALL:4>W1AW\n"
            << "<COMMEN"; // 故意截断
        return oss.str();
    }

    /** 长度不匹配：声明长度 > 实际数据 */
    static std::string makeLengthLongerThanData() {
        std::ostringstream oss;
        writeHeader(oss);
        oss << "<CALL:10>W1AW\n" // 声称 10 字符，实际只有 4
            << "<EOR>\n";
        return oss.str();
    }

    /** 长度不匹配：声明长度 < 实际数据 */
    static std::string makeLengthShorterThanData() {
        std::ostringstream oss;
        writeHeader(oss);
        oss << "<CALL:2>W1AW\n" // 声称 2，实际有 4
            << "<EOR>\n";
        return oss.str();
    }

    /** 缺失结束符：无 <EOR> 或 <EOH> */
    static std::string makeMissingTerminator() {
        std::ostringstream oss;
        // 故意不写 <EOH>
        oss << "ADIF 3.1.4\n"
            << "<CALL:4>W1AW\n"
            << "<QSO_DATE:8>20240315\n";
        // 没有 <EOR>，也没有 <EOH>
        return oss.str();
    }

    // ----------------------------- 辅助函数 -----------------------------

    /** 将生成的内容直接保存到文件 */
    static void saveToFile(const std::string &filename, const std::string &content) {
        std::ofstream out(filename, std::ios::binary);
        if (!out) {
            throw std::runtime_error("Cannot open file: " + filename);
        }
        out << content;
    }

  private:
    // 写入标准 ADIF 头部
    template <typename Stream> static void writeHeader(Stream &out) {
        out << "ADIF 3.1.4\n"
            << "Generated by ADIF Stress Test Generator\n"
            << "<EOH>\n";
    }

    // 随机生成一条 QSO（使用外部传入的随机引擎）
    template <typename Stream, typename RNG> static void appendRandomQSO(Stream &out, RNG &rng) {
        static const char *calls[] = {"W1AW", "K1JT", "DL1ABC", "JA1XYZ", "VK2ZZZ", "ZS1ABC"};
        static const char *bands[] = {"160M", "80M", "40M", "20M", "15M", "10M", "2M"};
        static const char *modes[] = {"SSB", "CW", "FM", "FT8", "RTTY", "PSK31"};

        std::uniform_int_distribution<size_t> callDist(0, 5);
        std::uniform_int_distribution<size_t> bandDist(0, 6);
        std::uniform_int_distribution<size_t> modeDist(0, 5);
        std::uniform_int_distribution<int> yearDist(2020, 2025);
        std::uniform_int_distribution<int> monthDist(1, 12);
        std::uniform_int_distribution<int> dayDist(1, 28);
        std::uniform_int_distribution<int> hourDist(0, 23);
        std::uniform_int_distribution<int> minuteDist(0, 59);
        std::uniform_real_distribution<double> freqDist(1.8, 450.0);

        // 构造日期时间字符串
        char dateBuf[16], timeBuf[16];
        snprintf(dateBuf, sizeof(dateBuf), "%04d%02d%02d", yearDist(rng), monthDist(rng),
                 dayDist(rng));
        snprintf(timeBuf, sizeof(timeBuf), "%02d%02d", hourDist(rng), minuteDist(rng));

        out << "<CALL:" << strlen(calls[callDist(rng)]) << '>' << calls[callDist(rng)] << '\n'
            << "<BAND:" << strlen(bands[bandDist(rng)]) << '>' << bands[bandDist(rng)] << '\n'
            << "<MODE:" << strlen(modes[modeDist(rng)]) << '>' << modes[modeDist(rng)] << '\n'
            << "<QSO_DATE:8>" << dateBuf << '\n'
            << "<TIME_ON:4>" << timeBuf << '\n';

        // 随机添加频率字段
        if (rng() % 2) {
            char freqBuf[32];
            snprintf(freqBuf, sizeof(freqBuf), "%.3f", freqDist(rng));
            out << "<FREQ:" << strlen(freqBuf) << '>' << freqBuf << '\n';
        }

        // 随机添加 RST 字段
        if (rng() % 2) {
            std::uniform_int_distribution<int> rstDist(111, 599);
            int rst = rstDist(rng);
            out << "<RST_SENT:3>" << rst << '\n';
        }
    }

    // 无参版本（使用默认随机引擎）
    template <typename Stream> static void appendRandomQSO(Stream &out) {
        static std::mt19937 rng(42);
        appendRandomQSO(out, rng);
    }
};

} // namespace adif_stress