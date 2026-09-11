#include "Respell.h"
#include "RespellLang.h"
#include "RespellUtil.h"

namespace
{
	using respell::CollapseWs;
	using respell::DecodeRunes;
	using respell::ExpectEq;
	using respell::MatchLowers;
	using respell::Rune;
	using respell::TitleCase;

	uint32_t FoldPolish(uint32_t Cp)
	{
		if (Cp >= 'A' && Cp <= 'Z')
			return Cp - 'A' + 'a';

		switch (Cp)
		{
		case 0x0104:
			return 0x0105; // Ą → ą
		case 0x0106:
			return 0x0107; // Ć → ć
		case 0x0118:
			return 0x0119; // Ę → ę
		case 0x0141:
			return 0x0142; // Ł → ł
		case 0x0143:
			return 0x0144; // Ń → ń
		case 0x00D3:
			return 0x00F3; // Ó → ó
		case 0x015A:
			return 0x015B; // Ś → ś
		case 0x0179:
			return 0x017A; // Ź → ź
		case 0x017B:
			return 0x017C; // Ż → ż
		default:
			return Cp;
		}
	}

	bool IsPolishAlpha(uint32_t Lower)
	{
		if (Lower >= 'a' && Lower <= 'z')
			return true;

		switch (Lower)
		{
		case 0x0105: // ą
		case 0x0107: // ć
		case 0x0119: // ę
		case 0x0142: // ł
		case 0x0144: // ń
		case 0x00F3: // ó
		case 0x015B: // ś
		case 0x017A: // ź
		case 0x017C: // ż
			return true;
		default:
			return false;
		}
	}

	bool IsIGlideVowel(uint32_t Lower)
	{
		switch (Lower)
		{
		case 'a':
		case 0x0105: // ą
		case 'e':
		case 0x0119: // ę
		case 'o':
		case 0x00F3: // ó
		case 'u':
			return true;
		default:
			return false;
		}
	}

	bool IsLabial(uint32_t Lower)
	{
		return Lower == 'b' || Lower == 'p';
	}

	bool IsVoicelessPolish(uint32_t Lower)
	{
		switch (Lower)
		{
		case 'p':
		case 't':
		case 'k':
		case 'c':
		case 's':
		case 'f':
		case 'h':
		case 0x0107: // ć
		case 0x015B: // ś
			return true;
		default:
			return false;
		}
	}

	bool IsWordFinalLetter(const std::vector<Rune> &Runes, size_t Index)
	{
		for (size_t j = Index + 1; j < Runes.size(); ++j)
		{
			if (IsPolishAlpha(Runes[j].lower))
				return false;
			return true;
		}
		return true;
	}

	const char *MapPolishNasalA(const std::vector<Rune> &Runes, size_t Index)
	{
		if (!IsWordFinalLetter(Runes, Index) && IsLabial(Runes[Index + 1].lower))
			return "ohm";
		return "ohn";
	}

	const char *MapPolishNasalE(const std::vector<Rune> &Runes, size_t Index)
	{
		if (IsWordFinalLetter(Runes, Index))
			return "eh";
		if (IsLabial(Runes[Index + 1].lower))
			return "ehm";
		return "ehn";
	}

	const char *MapPolishVowel(const std::vector<Rune> &Runes, size_t Index)
	{
		switch (Runes[Index].lower)
		{
		case 'a':
			return "ah";
		case 0x0105:
			return MapPolishNasalA(Runes, Index);
		case 'e':
			return "eh";
		case 0x0119:
			return MapPolishNasalE(Runes, Index);
		case 'i':
			return "ee";
		case 'o':
			return "oh";
		case 0x00F3:
			return "oo";
		case 'u':
			return "oo";
		case 'y':
			return "ih";
		default:
			return nullptr;
		}
	}

	const char *MapPolishIGlide(const std::vector<Rune> &Runes, size_t VowelIndex)
	{
		switch (Runes[VowelIndex].lower)
		{
		case 'a':
			return "yah";
		case 0x0105:
			return !IsWordFinalLetter(Runes, VowelIndex) && IsLabial(Runes[VowelIndex + 1].lower) ? "yohm"
																								: "yohn";
		case 'e':
			return "yeh";
		case 0x0119:
			if (IsWordFinalLetter(Runes, VowelIndex))
				return "yeh";
			return IsLabial(Runes[VowelIndex + 1].lower) ? "yehm" : "yen";
		case 'o':
			return "yoh";
		case 0x00F3:
		case 'u':
			return "yoo";
		default:
			return nullptr;
		}
	}

	const char *MapPolishLetter(uint32_t Lower)
	{
		switch (Lower)
		{
		case 'a':
			return "ah";
		case 'b':
			return "b";
		case 'c':
			return "ts";
		case 0x0107:
			return "ch"; // ć
		case 'd':
			return "d";
		case 'e':
			return "eh";
		case 'f':
			return "f";
		case 'g':
			return "g";
		case 'h':
			return "h";
		case 'i':
			return "ee";
		case 'j':
			return "y";
		case 'k':
			return "k";
		case 'l':
			return "l";
		case 0x0142:
			return "w"; // ł
		case 'm':
			return "m";
		case 'n':
			return "n";
		case 0x0144:
			return "n"; // ń
		case 'o':
			return "oh";
		case 0x00F3:
			return "oo"; // ó
		case 'p':
			return "p";
		case 'q':
			return "k";
		case 'r':
			return "r";
		case 's':
			return "s";
		case 0x015B:
			return "sh"; // ś
		case 't':
			return "t";
		case 'u':
			return "oo";
		case 'v':
			return "v";
		case 'w':
			return "v";
		case 'x':
			return "ks";
		case 'y':
			return "ih";
		case 'z':
			return "z";
		case 0x017A:
			return "zh"; // ź
		case 0x017C:
			return "zh"; // ż
		default:
			return nullptr;
		}
	}

	bool TryPalatalCluster(
		std::string &Out,
		const std::vector<Rune> &Runes,
		size_t &Index,
		uint32_t First,
		uint32_t Second,
		const char *Alone,
		const char *BeforeVowel)
	{
		if (Index + 1 >= Runes.size() || Runes[Index].lower != First || Runes[Index + 1].lower != Second)
			return false;

		if (Index + 2 < Runes.size() && IsIGlideVowel(Runes[Index + 2].lower))
		{
			if (BeforeVowel)
				Out += TitleCase(BeforeVowel, Runes[Index].upper);
			if (const char *vowel = MapPolishVowel(Runes, Index + 2))
				Out += TitleCase(vowel, Runes[Index + 2].upper);
			Index += 3;
			return true;
		}

		Out += TitleCase(Alone, Runes[Index].upper);
		Index += 2;
		return true;
	}

	std::string AdaptPolishForFonix(const std::string &Text)
	{
		const auto runes = DecodeRunes(Text, FoldPolish, IsPolishAlpha);
		std::string out;
		out.reserve(Text.size() * 2);

		for (size_t i = 0; i < runes.size();)
		{
			const uint32_t lower = runes[i].lower;

			// trz [tʂ] — closer to English "ch" than t+sh.
			static const uint32_t kTrz[] = { 't', 'r', 'z' };
			if (MatchLowers(runes, i, kTrz, 3))
			{
				out += TitleCase("ch", runes[i].upper);
				i += 3;
				continue;
			}

			static const uint32_t kDzi[] = { 'd', 'z', 'i' };
			if (MatchLowers(runes, i, kDzi, 3))
			{
				out += TitleCase("j", runes[i].upper);
				if (i + 3 < runes.size() && IsIGlideVowel(runes[i + 3].lower))
				{
					if (const char *vowel = MapPolishVowel(runes, i + 3))
						out += TitleCase(vowel, runes[i + 3].upper);
					i += 4;
				}
				else
				{
					out += TitleCase("ee", false);
					i += 3;
				}
				continue;
			}

			static const uint32_t kDz[] = { 'd', 'z' };
			static const uint32_t kDzh[] = { 'd', 0x017C }; // dż
			static const uint32_t kDzSoft[] = { 'd', 0x017A }; // dź
			static const uint32_t kCz[] = { 'c', 'z' };
			static const uint32_t kSz[] = { 's', 'z' };
			static const uint32_t kCh[] = { 'c', 'h' };
			static const uint32_t kRz[] = { 'r', 'z' };

			if (MatchLowers(runes, i, kDzh, 2) || MatchLowers(runes, i, kDzSoft, 2))
			{
				out += TitleCase("j", runes[i].upper);
				i += 2;
				continue;
			}
			if (MatchLowers(runes, i, kDz, 2))
			{
				out += TitleCase("dz", runes[i].upper);
				i += 2;
				continue;
			}
			if (MatchLowers(runes, i, kCz, 2))
			{
				out += TitleCase("ch", runes[i].upper);
				i += 2;
				continue;
			}
			if (MatchLowers(runes, i, kSz, 2))
			{
				out += TitleCase("sh", runes[i].upper);
				i += 2;
				continue;
			}
			if (MatchLowers(runes, i, kCh, 2))
			{
				out += TitleCase("h", runes[i].upper);
				i += 2;
				continue;
			}
			if (MatchLowers(runes, i, kRz, 2))
			{
				const bool voiceless = i > 0 && IsVoicelessPolish(runes[i - 1].lower);
				out += TitleCase(voiceless ? "sh" : "zh", runes[i].upper);
				i += 2;
				continue;
			}

			if (TryPalatalCluster(out, runes, i, 'c', 'i', "chee", "ch") ||
				TryPalatalCluster(out, runes, i, 's', 'i', "shee", "sh") ||
				TryPalatalCluster(out, runes, i, 'z', 'i', "zhee", "zh"))
			{
				continue;
			}

			if (i + 1 < runes.size() && lower == 'n' && runes[i + 1].lower == 'i')
			{
				if (i + 2 < runes.size() && IsIGlideVowel(runes[i + 2].lower))
				{
					if (const char *glide = MapPolishIGlide(runes, i + 2))
					{
						std::string mapped = "n";
						mapped += glide;
						out += TitleCase(mapped.c_str(), runes[i].upper);
					}
					i += 3;
					continue;
				}
				out += TitleCase("nee", runes[i].upper);
				i += 2;
				continue;
			}

			if (i + 1 < runes.size() && lower == 'i' && IsIGlideVowel(runes[i + 1].lower))
			{
				if (const char *glide = MapPolishIGlide(runes, i + 1))
					out += TitleCase(glide, runes[i].upper);
				i += 2;
				continue;
			}

			if (i + 1 < runes.size() && lower == 0x0144 && IsIGlideVowel(runes[i + 1].lower))
			{
				if (const char *glide = MapPolishIGlide(runes, i + 1))
				{
					std::string mapped = "n";
					mapped += glide;
					out += TitleCase(mapped.c_str(), runes[i].upper);
				}
				i += 2;
				continue;
			}

			if (lower == 0x0105)
			{
				out += TitleCase(MapPolishNasalA(runes, i), runes[i].upper);
				i += 1;
				continue;
			}
			if (lower == 0x0119)
			{
				out += TitleCase(MapPolishNasalE(runes, i), runes[i].upper);
				i += 1;
				continue;
			}

			if (const char *mapped = MapPolishLetter(lower))
			{
				out += TitleCase(mapped, runes[i].upper);
				i += 1;
				continue;
			}

			out.append(Text, runes[i].offset, runes[i].bytes);
			i += 1;
		}

		return CollapseWs(out);
	}

	int SelfTest()
	{
		int failed = 0;

		failed += !ExpectEq("tak", AdaptPolishForFonix("tak"), "tahk");
		failed += !ExpectEq("nie", AdaptPolishForFonix("nie"), "nyeh");
		failed += !ExpectEq("co", AdaptPolishForFonix("co"), "tsoh");
		failed += !ExpectEq("jest", AdaptPolishForFonix("jest"), "yehst");
		failed += !ExpectEq("czesc", AdaptPolishForFonix(u8"cześć"), "chehshch");
		failed += !ExpectEq("dziekuje", AdaptPolishForFonix(u8"dziękuję"), "jehnkooyeh");
		failed += !ExpectEq("sie", AdaptPolishForFonix(u8"się"), "sheh");
		failed += !ExpectEq("wszystko", AdaptPolishForFonix("wszystko"), "vshihstkoh");
		failed += !ExpectEq("przy", AdaptPolishForFonix("przy"), "pshih");
		failed += !ExpectEq("trzy", AdaptPolishForFonix("trzy"), "chih");
		failed += !ExpectEq("lodz", AdaptPolishForFonix(u8"łódź"), "wooj");
		failed += !ExpectEq("dzis", AdaptPolishForFonix(u8"dziś"), "jeesh");
		failed += !ExpectEq("dzien", AdaptPolishForFonix(u8"dzień"), "jehn");
		failed += !ExpectEq("ciocia", AdaptPolishForFonix("ciocia"), "chohchah");
		failed += !ExpectEq("mial", AdaptPolishForFonix(u8"miał"), "myahw");
		failed += !ExpectEq("sa", AdaptPolishForFonix(u8"są"), "sohn");
		failed += !ExpectEq("zeby", AdaptPolishForFonix(u8"zęby"), "zehmbih");
		failed += !ExpectEq("rak", AdaptPolishForFonix(u8"rąk"), "rohnk");
		failed += !ExpectEq("dziala", AdaptPolishForFonix(u8"działa"), "jahwah");
		failed += !ExpectEq("dobrze", AdaptPolishForFonix("dobrze"), "dohbzheh");
		failed += !ExpectEq("milosc", AdaptPolishForFonix(u8"miłość"), "meewohshch");
		failed += !ExpectEq("chleb", AdaptPolishForFonix("chleb"), "hlehb");
		failed += !ExpectEq("gora", AdaptPolishForFonix(u8"góra"), "goorah");
		failed += !ExpectEq("maly", AdaptPolishForFonix(u8"mały"), "mahwih");
		failed += !ExpectEq("chrzaszcz", AdaptPolishForFonix(u8"chrząszcz"), "hshohnshch");
		failed += !ExpectEq("patrz", AdaptPolishForFonix("patrz"), "pahch");
		failed += !ExpectEq(
			"prepare pl",
			PrepareDialogueText("Polish", u8"[Sarkazm] Cześć, przybyszu."),
			"Chehshch, pshihbihshoo.");
		failed += !ExpectEq(
			"usenglish passthrough",
			PrepareDialogueText("USEnglish", u8"cześć"),
			u8"cześć");
		failed += !ExpectEq("lang alias", ResolveFaceFxLanguage("Polish"), "USEnglish");
		failed += !ExpectEq("lang passthrough", ResolveFaceFxLanguage("French"), "French");
		return failed;
	}
}

void RegisterPolishRespell()
{
	static const RespellLanguage language = {
		"Polish",
		"pl",
		"USEnglish",
		u8"Cześć",
		AdaptPolishForFonix,
		SelfTest
	};
	RegisterRespellLanguage(language);
}
