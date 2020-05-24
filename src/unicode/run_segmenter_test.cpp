/**
 * This file is part of the "libunicode" project
 *   Copyright (c) 2020 Christian Parpart <christian@parpart.family>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <unicode/run_segmenter.h>
#include <unicode/utf8.h>
#include <unicode/ucd_ostream.h>

#include <array>
#include <string>
#include <string_view>

#include <catch2/catch.hpp>

#include <fmt/format.h>
#include <fmt/ostream.h>

using namespace std;
using namespace std::string_view_literals;
using namespace unicode;

namespace
{
    struct Expectation
    {
        u32string_view text;
        unicode::Script script;
        RunPresentationStyle presentationStyle;
    };

    void test_run_segmentation(std::vector<Expectation> const& _rs)
    {
        vector<segment> expects;
        u32string text;
        size_t i = 0;
        for (Expectation const& expect : _rs)
        {
            expects.push_back(segment{
                i,
                i + expect.text.size(),
                expect.script,
                expect.presentationStyle,
            });
            text += expect.text;
            i += expect.text.size();
        }

        auto segmenter = unicode::run_segmenter{text};
        unicode::segment actualSegment;
        for (size_t i = 0; i < _rs.size(); ++i)
        {
            INFO(fmt::format("run segmentation for part {}: \"{}\" to be {}",
                             i, to_utf8(_rs[i].text), expects[i]));
            REQUIRE(segmenter.consume(out(actualSegment)));

            CHECK(actualSegment == expects[i]);
        }
        REQUIRE_FALSE(segmenter.consume(out(actualSegment)));
    }
}

TEST_CASE("run_segmenter.empty", "[run_segmenter]")
{
    auto rs = run_segmenter{U""};
    auto result = segment{};
    auto const rv = rs.consume(out(result));
    CHECK_FALSE(rv);
    CHECK(result.start == 0);
    CHECK(result.end == 0);
    CHECK(result.script == Script::Unknown);
    CHECK(result.presentationStyle == RunPresentationStyle::Text);
}

TEST_CASE("run_segmenter.LatinPunctuationSideways", "[run_segmenter]")
{
    test_run_segmentation({
        {U"Abc.;?Xyz", Script::Latin, RunPresentationStyle::Text}
    });
}

TEST_CASE("run_segmenter.OneSpace", "[run_segmenter]")
{
    test_run_segmentation({
        {U" ", Script::Common, RunPresentationStyle::Text}
    });
}

TEST_CASE("run_segmenter.ArabicHangul", "[run_segmenter]")
{
    test_run_segmentation({
        {U"نص", Script::Arabic, RunPresentationStyle::Text},
        {U"키스의", Script::Hangul, RunPresentationStyle::Text}
    });
}

TEST_CASE("run_segmenter.JapaneseHindiEmojiMix", "[run_segmenter]")
{
    test_run_segmentation({
        {U"百家姓", Script::Han, RunPresentationStyle::Text},
        {U"ऋषियों", Script::Devanagari, RunPresentationStyle::Text},
        {U"🌱🌲🌳🌴", Script::Devanagari, RunPresentationStyle::Emoji},
        {U"百家姓", Script::Han, RunPresentationStyle::Text},
        {U"🌱🌲", Script::Han, RunPresentationStyle::Emoji}
    });
}

TEST_CASE("run_segmenter.CombiningCirlce", "[run_segmenter]")
{
    test_run_segmentation({
        {U"◌́◌̀◌̈◌̂◌̄◌̊", Script::Common, RunPresentationStyle::Text}
    });
}

TEST_CASE("run_segmenter.Arab_Hangul", "[run_segmenter]")
{
    test_run_segmentation({
        {U"نص", Script::Arabic, RunPresentationStyle::Text},
        {U"키스의",  Script::Hangul, RunPresentationStyle::Text},
    });
}

// TODO: orientation
// TEST_CASE("run_segmenter.HangulSpace", "[run_segmenter]")
// {
//     test_run_segmentation({
//         {U"키스의", Script::Hangul, RunPresentationStyle::Text},     // Orientation::Keep
//         {U" ", Script::Hangul, RunPresentationStyle::Text},          // Orientation::Sideways
//         {U"고유조건은", Script::Hangul, RunPresentationStyle::Text}  // Orientation::Keep
//     });
// }

TEST_CASE("run_segmenter.TechnicalCommonUpright", "[run_segmenter]")
{
    test_run_segmentation({
        {U"⌀⌁⌂", Script::Common, RunPresentationStyle::Text},
    });
}

TEST_CASE("run_segmenter.PunctuationCommonSideways", "[run_segmenter]")
{
    test_run_segmentation({
        {U".…¡", Script::Common, RunPresentationStyle::Text}
    });
}

// TODO: orientation
// TEST_CASE("run_segmenter.JapanesePunctuationMixedInside", "[run_segmenter]")
// {
//     test_run_segmentation({
//         {U"いろはに", Script::Hiragana, RunPresentationStyle::Text}, // Orientation::Keep
//         {U".…¡", Script::Hiragana, RunPresentationStyle::Text},      // Orientation::RotateSideways
//         {U"ほへと", Script::Hiragana, RunPresentationStyle::Text},   // Orientation::Keep
//     });
// }

TEST_CASE("run_segmenter.JapanesePunctuationMixedInsideHorizontal", "[run_segmenter]")
{
    test_run_segmentation({
        {U"いろはに.…¡ほへと", Script::Hiragana, RunPresentationStyle::Text}, // Orientation::Keep
    });
}

TEST_CASE("run_segmenter.PunctuationDevanagariCombining", "[run_segmenter]")
{
    test_run_segmentation({
        {U"क+े", Script::Devanagari, RunPresentationStyle::Text}, // Orientation::Keep
    });
}

TEST_CASE("run_segmenter.EmojiZWJSequences", "[run_segmenter]")
{
    test_run_segmentation({
        {U"👩‍👩‍👧‍👦👩‍❤️‍💋‍👨", Script::Latin, RunPresentationStyle::Emoji},
        {U"abcd", Script::Latin, RunPresentationStyle::Text},
        {U"👩‍👩", Script::Latin, RunPresentationStyle::Emoji},
        {U"\U0000200D‍efg", Script::Latin, RunPresentationStyle::Text},
    });
}

// TODO: Orientation
// TEST_CASE("run_segmenter.JapaneseLetterlikeEnd", "[run_segmenter]")
// {
//     test_run_segmentation({
//         {U"いろは", Script::Hiragana, RunPresentationStyle::Text}, // Orientation::Keep
//         {U"ℐℒℐℒℐℒℐℒℐℒℐℒℐℒ", Script::Hiragana, RunPresentationStyle::Text}, // Orientation::RotateSideways
//     });
// }

// TODO: Orientation
// TEST_CASE("run_segmenter.JapaneseCase", "[run_segmenter]")
// {
//     test_run_segmentation({
//         {U"いろは", Script::Hiragana, RunPresentationStyle::Text},   // Keep
//         {U"aaAA", Script::Latin, RunPresentationStyle::Text},        // RotateSideways
//         {U"いろは", Script::Hiragana, RunPresentationStyle::Text},   // Keep
//     });
// }

TEST_CASE("run_segmenter.DingbatsMiscSymbolsModifier", "[run_segmenter]")
{
    test_run_segmentation({{
        U"⛹🏻✍🏻✊🏼",
        Script::Common,
        RunPresentationStyle::Emoji
    }});
}

TEST_CASE("run_segmenter.ArmenianCyrillicCase", "[run_segmenter]")
{
    test_run_segmentation({
        {U"աբգ", Script::Armenian, RunPresentationStyle::Text},
        {U"αβγ", Script::Greek, RunPresentationStyle::Text},
        {U"ԱԲԳ", Script::Armenian, RunPresentationStyle::Text}
    });
}

TEST_CASE("run_segmenter.EmojiSubdivisionFlags", "[run_segmenter]")
{
    test_run_segmentation({{
        U"🏴󠁧󠁢󠁷󠁬󠁳󠁿🏴󠁧󠁢󠁳󠁣󠁴󠁿🏴󠁧󠁢"
        U"󠁥󠁮󠁧󠁿",
        Script::Common,
        RunPresentationStyle::Emoji
    }});
}

TEST_CASE("run_segmenter.NonEmojiPresentationSymbols", "[run_segmenter]")
{
    test_run_segmentation({{
        U"\U00002626\U0000262a\U00002638\U0000271d\U00002721\U00002627"
        U"\U00002628\U00002629\U0000262b\U0000262c\U00002670"
        U"\U00002671\U0000271f\U00002720",
        Script::Common,
        RunPresentationStyle::Text
    }}); // Orientation::Keep
}
