/*
 * Copyright 2017 Rowun Giles - http://github.com/rowungiles
 */

#include <chrono>
#include "gtest/gtest.h"
#include "heckel_diff.hpp"
#include "helpers.hpp"

template <typename T>
void checkExpectedType(const std::vector<T> *result, const std::vector<T> &actual) {

    if (result == nullptr) {
        return;
    }

    auto expected = *result;

    EXPECT_EQ(expected.size(), actual.size());

    for (size_t i = 0; i < actual.size(); ++i) {
        EXPECT_EQ(expected[i], actual[i]);
    }
}

/*
 * TODO: rather than points, optionals better represent the intent of expected.
 * Boost 1.63 was failing on macOS Sierra - try again when 1.64 is out.
 */

template <typename T>
void testExpectations(const std::vector<T> &original, const std::vector<T> &updated,
                      std::vector<T> *expected_inserted,
                      std::vector<T> *expected_deleted,
                      std::vector<T> *expected_moved,
                      std::vector<T> *expected_unchanged,
                      bool is_timed = false,
                      double not_greater_than_ms_time = 0.0) {

    HeckelDiff::Algorithm<T> h;

    std::unordered_map<std::string, std::vector<T>> actual;

    if (is_timed) {

        uint32_t samples = 10;

        auto sum_cputime_ms = 0.0;
        auto sum_wallclock_ms = 0.0;

        for (uint32_t i=0; i < samples; i+=1) {

            std::clock_t c_start = std::clock();
            auto t_start = std::chrono::steady_clock::now();

            actual = h.diff(original, updated);

            auto t_end = std::chrono::steady_clock::now();
            std::clock_t c_end = std::clock();

            sum_cputime_ms += 1000.0 * (c_end - c_start) / CLOCKS_PER_SEC;
            sum_wallclock_ms += std::chrono::duration<double, std::milli>(t_end - t_start).count();
        }

        std::cout << "\n"
                  << "Not greater than: "
                  << not_greater_than_ms_time
                  << " ms\n"
                  << std::fixed << std::setprecision(2) << "CPU time used: "
                  << (sum_cputime_ms / samples)
                  << " ms\n"
                  << "Wall clock time passed: "
                  << (sum_wallclock_ms / samples)
                  << " ms\n";

        EXPECT_GE(not_greater_than_ms_time, (sum_wallclock_ms / samples));

    } else {

        actual = h.diff(original, updated);
    }

    auto actual_inserted = actual[HeckelDiff::INSERTED];
    auto actual_deleted = actual[HeckelDiff::DELETED];
    auto actual_moved = actual[HeckelDiff::MOVED];
    auto actual_unchanged = actual[HeckelDiff::UNCHANGED];

    checkExpectedType<T>(expected_inserted, actual_inserted);
    checkExpectedType<T>(expected_deleted, actual_deleted);
    checkExpectedType<T>(expected_moved, actual_moved);
    checkExpectedType<T>(expected_unchanged, actual_unchanged);
}

TEST(HeckelDiff, CharactersInserted) {

    std::vector<std::string> original {"A", "X", "C", "Y", "D", "W", "E", "A", "E"};
    std::vector<std::string> updated {"A", "B", "C", "D", "E"};

    auto expected_inserted = new std::vector<std::string> {"B"};

    testExpectations<std::string>(original, updated, expected_inserted, nullptr, nullptr, nullptr);

    delete expected_inserted;
}

TEST(HeckelDiff, CharactersDeleted) {

    std::vector<std::string> original {"A", "X", "C", "Y", "D", "W", "E", "A", "E"};
    std::vector<std::string> updated {"A", "B", "C", "D", "E"};

    auto expected_deleted = new std::vector<std::string> {"X", "Y", "W", "A", "E"};

    testExpectations<std::string>(original, updated, nullptr, expected_deleted, nullptr, nullptr);

    delete expected_deleted;
}

TEST(HeckelDiff, CharactersMoved) {

    std::vector<std::string> original {"A", "X", "C", "Y", "D", "W", "E", "A", "E"};
    std::vector<std::string> updated {"A", "B", "C", "D", "E"};

    auto expected_moved = new std::vector<std::string> {"A", "D", "E"};

    testExpectations<std::string>(original, updated, nullptr, nullptr, expected_moved, nullptr);

    delete expected_moved;
}

TEST(HeckelDiff, CharactersUnchanged) {

    std::vector<std::string> original {"A", "X", "C", "Y", "D", "W", "E", "A", "E"};
    std::vector<std::string> updated {"A", "B", "C", "D", "E"};

    auto expected_unchanged = new std::vector<std::string> {"C"};

    testExpectations<std::string>(original, updated, nullptr, nullptr, nullptr, expected_unchanged);

    delete expected_unchanged;
}

TEST(HeckelDiff, NoChange) {

    std::vector<std::string> original {"A", "X", "C", "Y", "D", "W", "E", "A", "E"};
    std::vector<std::string> updated {"A", "X", "C", "Y", "D", "W", "E", "A", "E"};

    auto expected_unchanged = new std::vector<std::string> {"A", "X", "C", "Y", "D", "W", "E", "A", "E"};

    testExpectations<std::string>(original, updated, nullptr, nullptr, nullptr, expected_unchanged);

    delete expected_unchanged;
}

TEST(HeckelDiff, CompleteChangeUpdate) {

    std::vector<std::string> original {};
    std::vector<std::string> updated {"A", "X", "C", "Y", "D", "W", "E", "A", "E"};

    auto expected_inserted = new std::vector<std::string> {"A", "X", "C", "Y", "D", "W", "E", "A", "E"};

    testExpectations<std::string>(original, updated, expected_inserted, nullptr, nullptr, nullptr);

    delete expected_inserted;
}

TEST(HeckelDiff, CompleteChangeOriginal) {

    std::vector<std::string> original {"A", "X", "C", "Y", "D", "W", "E", "A", "E"};
    std::vector<std::string> updated {};

    auto expected_deleted = new std::vector<std::string> {"A", "X", "C", "Y", "D", "W", "E", "A", "E"};

    testExpectations<std::string>(original, updated, nullptr, expected_deleted, nullptr, nullptr);

    delete expected_deleted;
}

TEST(HeckelDiff, BlockMove) {

    std::vector<std::string> original{"A", "B", "C", "D", "E", "F", "G", "H"};
    std::vector<std::string> updated{"E", "F", "G", "H", "A", "B", "C", "D"};

    auto expected_moved = new std::vector<std::string>{"E", "F", "G", "H", "A", "B", "C", "D"};

    testExpectations<std::string>(original, updated, nullptr, nullptr, expected_moved, nullptr);

    delete expected_moved;
}

TEST(HeckelDiff, Benchmark) {

    std::vector<size_t> original{};

    for (size_t i = 0; i < 16000; i+=1) {
        original.push_back(i);
    }

    std::vector<size_t> updated = original;

    std::reverse(original.begin(), original.end());

    auto one_frame = 16.67;  // 60fps
    testExpectations<size_t>(original, updated, nullptr, nullptr, nullptr, nullptr, true, one_frame);
}

//  MARK: Mixture of Scenarios
TEST(HeckelDiff, MixtureOfAllScenariosAndVariableLength) {

    std::vector<std::string> original {"A", "X", "C", "Y", "D", "W", "E", "A", "E"};
    std::vector<std::string> updated {"A", "B", "C", "D", "E", "A", "Y", "Y"};

    auto expected_deleted = new std::vector<std::string> {"X", "W", "E"};
    auto expected_inserted = new std::vector<std::string> {"B", "Y"};
    auto expected_moved = new std::vector<std::string> {"D", "E", "A", "Y"};
    auto expected_unchanged = new std::vector<std::string> {"A", "C"};

    testExpectations<std::string>(original, updated, expected_inserted,
                                  expected_deleted,
                                  expected_moved,
                                  expected_unchanged);

    delete expected_deleted;
}

static std::vector<std::string> delimited_reference_manual_o() {
    const std::string reference_manual_o = "much writing is like snow , a mass of long words and phrases falls upon "
            "the relevant facts covering up the details .";
    return HeckelDiffHelpers::components_seperated_by_delimiter(reference_manual_o, ' ');
}

static std::vector<std::string> delimited_reference_manual_n() {
    const std::string reference_manual_n = "a mass of latin words falls upon the relevant facts like soft snow , "
            "covering up the details .";
    return HeckelDiffHelpers::components_seperated_by_delimiter(reference_manual_n, ' ');
}

// MARK: Testing the reference manual - http://documents.scribd.com/docs/10ro9oowpo1h81pgh1as.pdf
TEST(HeckelDiff, ReferenceManualInserted) {

    std::vector<std::string> original = delimited_reference_manual_o();
    std::vector<std::string> updated = delimited_reference_manual_n();

    auto expected_inserted = new std::vector<std::string> {"latin", "soft"};

    testExpectations<std::string>(original, updated, expected_inserted, nullptr, nullptr, nullptr);

    delete expected_inserted;
}

TEST(HeckelDiff, ReferenceManualDeleted) {

    std::vector<std::string> original = delimited_reference_manual_o();
    std::vector<std::string> updated = delimited_reference_manual_n();

    auto expected_deleted = new std::vector<std::string> {"much", "writing", "is", "long", "and", "phrases"};

    testExpectations<std::string>(original, updated, nullptr, expected_deleted, nullptr, nullptr);

    delete expected_deleted;
}

TEST(HeckelDiff, ReferenceManualMoved) {

    std::vector<std::string> original = delimited_reference_manual_o();
    std::vector<std::string> updated = delimited_reference_manual_n();

    auto expected_moved = new std::vector<std::string> {"a", "mass", "of", "words", "falls", "upon", "the",
                                                        "relevant", "facts", "like", "snow", ",", "covering", "up",
                                                        "the", "details", "."};

    testExpectations<std::string>(original, updated, nullptr, nullptr, expected_moved, nullptr);

    delete expected_moved;
}

TEST(HeckelDiff, ReferenceManualUnchanged) {

    std::vector<std::string> original = delimited_reference_manual_o();
    std::vector<std::string> updated = delimited_reference_manual_n();

    auto expected_unchanged = new std::vector<std::string> {};

    testExpectations<std::string>(original, updated, nullptr, nullptr, nullptr, expected_unchanged);

    delete expected_unchanged;
}

// Utilise several tests from IGListKit (https://github.com/Instagram/IGListKit) for more completeness.
TEST(HeckelDiff, IGListKitWhenDiffingEmptyArraysThatResultHasNoChanges) {

    std::vector<size_t> original {};
    std::vector<size_t> updated {};

    auto expected = new std::vector<size_t> {};

    testExpectations<size_t>(original, updated, expected, expected, expected, expected);

    delete expected;
}

TEST(HeckelDiff, IGListKitWhenDiffingFromEmptyArrayThatResultHasChanges) {

    std::vector<size_t> original {};
    std::vector<size_t> updated {1};

    auto expected = new std::vector<size_t> {1};

    testExpectations<size_t>(original, updated, expected, nullptr, nullptr, nullptr);

    delete expected;
}

TEST(HeckelDiff, IGListKitWhenSwappingObjectsThatResultHasMoves) {

    std::vector<size_t> original {1, 2};
    std::vector<size_t> updated {2, 1};

    auto expected = new std::vector<size_t> {2, 1};

    testExpectations<size_t>(original, updated, nullptr, nullptr, expected, nullptr);

    delete expected;
}

TEST(HeckelDiff, IGListKitWhenMovingObjectsTogetherThatResultHasMoves) {

    std::vector<size_t> original {1, 2, 3, 3, 4};
    std::vector<size_t> updated {2, 3, 1, 3, 4};

    auto expected = new std::vector<size_t> {2, 3, 1};

    testExpectations<size_t>(original, updated, nullptr, nullptr, expected, nullptr);

    delete expected;
}

TEST(HeckelDiff, IGListKitWhenDeletingItemsWithInsertsWithMovesThatResultHasInsertsMovesAndDeletes) {

    std::vector<size_t> original {0, 1, 2, 3, 4, 5, 6, 7, 8};
    std::vector<size_t> updated  {0, 2, 3, 4, 7, 6, 9, 5, 10};

    auto expected_inserted = new std::vector<size_t> {9, 10};
    auto expected_deleted = new std::vector<size_t> {1, 8};
    auto expected_moved = new std::vector<size_t> {2, 3, 4, 7, 6, 5};

    testExpectations<size_t>(original, updated, expected_inserted, expected_deleted, expected_moved, nullptr);

    delete expected_inserted;
    delete expected_deleted;
    delete expected_moved;
}

TEST(HeckelDiff, IGListKitWhenInsertingObjectsWithArrayOfEqualObjectsThatChangeCountMatches) {

    std::vector<std::string> original {"dog", "dog"};
    std::vector<std::string> updated  {"dog", "dog", "dog", "dog"};

    auto expected = new std::vector<std::string> {"dog", "dog"};

    testExpectations<std::string>(original, updated, expected, nullptr, nullptr, nullptr);

    delete expected;
}

TEST(HeckelDiff, IGListKitWhenDeletingObjectsWithArrayOfEqualObjectsThatChangeCountMatches) {

    std::vector<std::string> original {"dog", "dog", "dog", "dog"};
    std::vector<std::string> updated  {"dog", "dog"};

    auto expected = new std::vector<std::string> {"dog", "dog"};

    testExpectations<std::string>(original, updated, nullptr, expected, nullptr, nullptr);

    delete expected;
}
