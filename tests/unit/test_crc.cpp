#include "../framework/test_framework.h"
#include "../mocks/MockEnvironment.h"
#include "../../src/crc.h"
#include <cstring>

TEST_CASE(CRC32, StandardVector_123456789) {
    const char* input = "123456789";
    unsigned int result = crc32_buf(input, (unsigned long)strlen(input));
    // Standard IEEE 802.3 CRC-32 checksum for "123456789" is 0xCBF43926
    ASSERT_EQ(result, 0xCBF43926u);
}

TEST_CASE(CRC32, EmptyBuffer) {
    const char* input = "";
    unsigned int result = crc32_buf(input, 0);
    ASSERT_EQ(result, 0x00000000u);
}

TEST_CASE(CRC32, SingleCharacter) {
    const char* a = "a";
    unsigned int result_a = crc32_buf(a, 1);
    ASSERT_EQ(result_a, 0xE8B7BE43u);

    const char* z = "Z";
    unsigned int result_z = crc32_buf(z, 1);
    ASSERT_EQ(result_z, 0x59BC5767u);
}

TEST_CASE(CRC32, StandardPangram_TheQuickBrownFox) {
    const char* pangram = "The quick brown fox jumps over the lazy dog";
    unsigned int result = crc32_buf(pangram, (unsigned long)strlen(pangram));
    ASSERT_EQ(result, 0x414FA339u);
}

TEST_CASE(CRC32, BinaryBufferWithNullBytes) {
    const char data[] = {'C', 'u', 'b', 'e', '\0', 'W', 'o', 'r', 'l', 'd', '\0', 'M', 'o', 'd'};
    unsigned long len = sizeof(data);
    unsigned int result = crc32_buf(data, len);
    ASSERT_NE(result, 0x00000000u);

    // Consistency check: running again yields the exact same CRC
    unsigned int result2 = crc32_buf(data, len);
    ASSERT_EQ(result, result2);
}

TEST_CASE(CRC32, LargeBufferConsistency) {
    std::vector<char> large_buf(65536, 'X');
    for (size_t i = 0; i < large_buf.size(); ++i) {
        large_buf[i] = static_cast<char>(i % 256);
    }

    unsigned int result1 = crc32_buf(large_buf.data(), (unsigned long)large_buf.size());
    unsigned int result2 = crc32_buf(large_buf.data(), (unsigned long)large_buf.size());
    ASSERT_NE(result1, 0x00000000u);
    ASSERT_EQ(result1, result2);
}

TEST_CASE(CRC32, FileCrcCalculation) {
    Mocking::TempDirectoryScope temp_dir;
    auto file_path = temp_dir.create_file("test_crc.bin", "123456789");

    unsigned int file_crc = crc32_file(file_path.string().c_str());
    ASSERT_EQ(file_crc, 0xCBF43926u);
}

TEST_CASE(CRC32, FileEmpty) {
    Mocking::TempDirectoryScope temp_dir;
    auto file_path = temp_dir.create_file("empty.bin", "");

    unsigned int file_crc = crc32_file(file_path.string().c_str());
    ASSERT_EQ(file_crc, 0x00000000u);
}

TEST_CASE(CRC32, FileBinaryPayload) {
    Mocking::TempDirectoryScope temp_dir;
    std::string payload;
    for (int i = 0; i < 1024; ++i) {
        payload.push_back(static_cast<char>(i & 0xFF));
    }
    auto file_path = temp_dir.create_file("payload.bin", payload);

    unsigned int mem_crc = crc32_buf(payload.data(), (unsigned long)payload.size());
    unsigned int file_crc = crc32_file(file_path.string().c_str());

    ASSERT_EQ(file_crc, mem_crc);
}

TEST_CASE(CRC32, FileNullPointerSafe) {
    unsigned int crc = crc32_file(nullptr);
    ASSERT_EQ(crc, 0x00000000u);
}

TEST_CASE(CRC32, FileNonExistentReturnsZero) {
    Mocking::TempDirectoryScope temp_dir;
    auto missing_path = temp_dir.path / "missing_file_xyz.bin";
    unsigned int crc = crc32_file(missing_path.string().c_str());
    ASSERT_EQ(crc, 0x00000000u);
}

