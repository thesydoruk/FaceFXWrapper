#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace respell
{
	struct Codepoint
	{
		uint32_t cp = 0;
		size_t bytes = 1;
	};

	inline Codepoint DecodeUtf8(const std::string &Text, size_t Index)
	{
		const unsigned char lead = static_cast<unsigned char>(Text[Index]);
		if (lead < 0x80)
			return { lead, 1 };

		if ((lead & 0xE0) == 0xC0 && Index + 1 < Text.size())
		{
			return { static_cast<uint32_t>(((lead & 0x1F) << 6) |
					(static_cast<unsigned char>(Text[Index + 1]) & 0x3F)),
				2 };
		}

		if ((lead & 0xF0) == 0xE0 && Index + 2 < Text.size())
		{
			return { static_cast<uint32_t>(((lead & 0x0F) << 12) |
					((static_cast<unsigned char>(Text[Index + 1]) & 0x3F) << 6) |
					(static_cast<unsigned char>(Text[Index + 2]) & 0x3F)),
				3 };
		}

		if ((lead & 0xF8) == 0xF0 && Index + 3 < Text.size())
		{
			return { static_cast<uint32_t>(((lead & 0x07) << 18) |
					((static_cast<unsigned char>(Text[Index + 1]) & 0x3F) << 12) |
					((static_cast<unsigned char>(Text[Index + 2]) & 0x3F) << 6) |
					(static_cast<unsigned char>(Text[Index + 3]) & 0x3F)),
				4 };
		}

		return { lead, 1 };
	}

	struct Rune
	{
		uint32_t cp = 0;
		uint32_t lower = 0;
		bool upper = false;
		size_t offset = 0;
		size_t bytes = 1;
	};

	inline std::vector<Rune> DecodeRunes(
		const std::string &Text,
		uint32_t (*Fold)(uint32_t),
		bool (*IsAlpha)(uint32_t))
	{
		std::vector<Rune> runes;
		runes.reserve(Text.size());
		for (size_t i = 0; i < Text.size();)
		{
			const auto decoded = DecodeUtf8(Text, i);
			Rune rune;
			rune.cp = decoded.cp;
			rune.lower = Fold(decoded.cp);
			rune.upper = IsAlpha(rune.lower) && rune.lower != decoded.cp;
			rune.offset = i;
			rune.bytes = decoded.bytes;
			runes.push_back(rune);
			i += decoded.bytes;
		}
		return runes;
	}

	inline bool MatchLowers(const std::vector<Rune> &Runes, size_t Index, const uint32_t *Lowers, size_t Count)
	{
		if (Index + Count > Runes.size())
			return false;
		for (size_t i = 0; i < Count; ++i)
		{
			if (Runes[Index + i].lower != Lowers[i])
				return false;
		}
		return true;
	}

	inline std::string CollapseWs(const std::string &Text)
	{
		std::string out;
		out.reserve(Text.size());
		bool space = false;
		for (unsigned char ch : Text)
		{
			if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
			{
				space = true;
				continue;
			}
			if (space && !out.empty())
				out.push_back(' ');
			space = false;
			out.push_back(static_cast<char>(ch));
		}
		return out;
	}

	inline std::string SanitizeDialogueText(const char *Text)
	{
		const std::string src = Text ? Text : "";
		std::string out;
		out.reserve(src.size());

		for (size_t i = 0; i < src.size();)
		{
			if (src[i] == '[')
			{
				const size_t close = src.find(']', i + 1);
				if (close != std::string::npos)
				{
					out.push_back(' ');
					i = close + 1;
					continue;
				}
				out.push_back(' ');
				i += 1;
				continue;
			}

			if (src[i] == ']')
			{
				out.push_back(' ');
				i += 1;
				continue;
			}

			out.push_back(src[i]);
			i += 1;
		}

		return CollapseWs(out);
	}

	inline std::string TitleCase(const char *Mapped, bool Upper)
	{
		if (!Mapped || !Mapped[0])
			return {};
		std::string out = Mapped;
		if (Upper && out[0] >= 'a' && out[0] <= 'z')
			out[0] = static_cast<char>(out[0] - 'a' + 'A');
		return out;
	}

	inline bool ExpectEq(const char *Name, const std::string &Got, const char *Want)
	{
		if (Got == Want)
		{
			printf("  ok  %s\n", Name);
			return true;
		}
		printf("  FAIL %s\n       got  \"%s\"\n       want \"%s\"\n", Name, Got.c_str(), Want);
		return false;
	}
}
