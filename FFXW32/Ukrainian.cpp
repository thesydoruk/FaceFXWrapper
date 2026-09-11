#include "Ukrainian.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace
{
	struct Codepoint
	{
		uint32_t cp = 0;
		size_t bytes = 1;
	};

	Codepoint DecodeUtf8(const std::string &Text, size_t Index)
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

	bool IsCyrillic(uint32_t Cp)
	{
		return Cp >= 0x0400 && Cp <= 0x04FF;
	}

	uint32_t ToLowerCp(uint32_t Cp)
	{
		if (Cp >= 0x0410 && Cp <= 0x042F)
			return Cp + 0x20;

		switch (Cp)
		{
		case 0x0401:
			return 0x0451; // Ё → ё
		case 0x0404:
			return 0x0454; // Є → є
		case 0x0406:
			return 0x0456; // І → і
		case 0x0407:
			return 0x0457; // Ї → ї
		case 0x0490:
			return 0x0491; // Ґ → ґ
		default:
			return Cp;
		}
	}

	bool IsUpperCp(uint32_t Cp)
	{
		return ToLowerCp(Cp) != Cp;
	}

	bool IsApostrophe(uint32_t Cp)
	{
		return Cp == '\'' || Cp == '`' || Cp == 0x2019 || Cp == 0x02BC;
	}

	const char *MapLetter(uint32_t Lower)
	{
		switch (Lower)
		{
		case 0x0430:
			return "ah"; // а
		case 0x0431:
			return "b"; // б
		case 0x0432:
			return "v"; // в
		case 0x0433:
			return "h"; // г
		case 0x0491:
			return "g"; // ґ
		case 0x0434:
			return "d"; // д
		case 0x0435:
			return "eh"; // е
		case 0x0454:
			return "yeh"; // є
		case 0x0436:
			return "zh"; // ж
		case 0x0437:
			return "z"; // з
		case 0x0438:
			return "ih"; // и
		case 0x0456:
			return "ee"; // і
		case 0x0457:
			return "yee"; // ї
		case 0x0439:
			return "y"; // й
		case 0x043A:
			return "k"; // к
		case 0x043B:
			return "l"; // л
		case 0x043C:
			return "m"; // м
		case 0x043D:
			return "n"; // н
		case 0x043E:
			return "oh"; // о
		case 0x043F:
			return "p"; // п
		case 0x0440:
			return "r"; // р
		case 0x0441:
			return "s"; // с
		case 0x0442:
			return "t"; // т
		case 0x0443:
			return "oo"; // у
		case 0x0444:
			return "f"; // ф
		case 0x0445:
			return "h"; // х — Fonix reads "kh" as K; HH matches [x] better.
		case 0x0446:
			return "ts"; // ц
		case 0x0447:
			return "ch"; // ч
		case 0x0448:
			return "sh"; // ш
		case 0x0449:
			return "shch"; // щ
		case 0x044C:
			return ""; // ь
		case 0x044E:
			return "yoo"; // ю
		case 0x044F:
			return "yah"; // я
		case 0x044A:
			return ""; // ъ
		case 0x044B:
			return "ih"; // ы
		case 0x044D:
			return "eh"; // э
		case 0x0451:
			return "yoh"; // ё
		default:
			return nullptr;
		}
	}

	std::string CollapseWs(const std::string &Text)
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

	std::string SanitizeDialogueText(const char *Text)
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

	std::string TitleCase(const char *Mapped, bool Upper)
	{
		if (!Mapped || !Mapped[0])
			return {};
		std::string out = Mapped;
		if (Upper && out[0] >= 'a' && out[0] <= 'z')
			out[0] = static_cast<char>(out[0] - 'a' + 'A');
		return out;
	}

	bool HasCyrillic(const std::string &Text)
	{
		for (size_t i = 0; i < Text.size();)
		{
			const auto decoded = DecodeUtf8(Text, i);
			if (IsCyrillic(decoded.cp))
				return true;
			i += decoded.bytes;
		}
		return false;
	}

	std::string AdaptUkrainianForFonix(const std::string &Text)
	{
		if (!HasCyrillic(Text))
			return Text;

		std::string out;
		out.reserve(Text.size() * 2);

		for (size_t i = 0; i < Text.size();)
		{
			const auto first = DecodeUtf8(Text, i);
			const uint32_t lower = ToLowerCp(first.cp);

			if (i + first.bytes < Text.size())
			{
				const auto second = DecodeUtf8(Text, i + first.bytes);
				const uint32_t pair = ToLowerCp(second.cp);
				if (lower == 0x044C && pair == 0x043E) // ьо
				{
					out += TitleCase("yo", IsUpperCp(first.cp));
					i += first.bytes + second.bytes;
					continue;
				}
				if (lower == 0x0439 && pair == 0x043E) // йо
				{
					out += TitleCase("yo", IsUpperCp(first.cp));
					i += first.bytes + second.bytes;
					continue;
				}
				if (lower == 0x0434 && pair == 0x0436) // дж
				{
					out += TitleCase("j", IsUpperCp(first.cp));
					i += first.bytes + second.bytes;
					continue;
				}
				if (lower == 0x0434 && pair == 0x0437) // дз
				{
					out += TitleCase("dz", IsUpperCp(first.cp));
					i += first.bytes + second.bytes;
					continue;
				}
			}

			if (IsApostrophe(first.cp))
			{
				i += first.bytes;
				continue;
			}

			if (const char *mapped = MapLetter(lower))
			{
				out += TitleCase(mapped, IsUpperCp(first.cp));
				i += first.bytes;
				continue;
			}

			out.append(Text, i, first.bytes);
			i += first.bytes;
		}

		return CollapseWs(out);
	}

	bool ExpectEq(const char *Name, const std::string &Got, const char *Want)
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

bool IsUkrainianLanguage(const char *Language)
{
	return Language && _stricmp(Language, "Ukrainian") == 0;
}

std::string PrepareDialogueText(const char *Language, const char *Text)
{
	if (!IsUkrainianLanguage(Language))
		return Text ? Text : "";
	return AdaptUkrainianForFonix(SanitizeDialogueText(Text));
}

const char *ResolveFaceFxLanguage(const char *Language)
{
	if (IsUkrainianLanguage(Language))
		return "USEnglish";
	return Language;
}

int RunUkrainianSelfTest()
{
	int failed = 0;

	printf("FaceFXWrapper Ukrainian self-test (Fonix respell)\n");

	failed += !ExpectEq("ascii", AdaptUkrainianForFonix("Hello, vault dweller."), "Hello, vault dweller.");
	failed += !ExpectEq("pryvit", AdaptUkrainianForFonix(u8"привіт"), "prihveet");
	failed += !ExpectEq("ni", AdaptUkrainianForFonix(u8"ні"), "nee");
	failed += !ExpectEq("tse", AdaptUkrainianForFonix(u8"це"), "tseh");
	failed += !ExpectEq("shcho", AdaptUkrainianForFonix(u8"що"), "shchoh");
	failed += !ExpectEq("yizha", AdaptUkrainianForFonix(u8"їжа"), "yeezhah");
	failed += !ExpectEq("gava", AdaptUkrainianForFonix(u8"ґава"), "gahvah");
	failed += !ExpectEq("dyakuyu", AdaptUkrainianForFonix(u8"дякую"), "dyahkooyoo");
	failed += !ExpectEq("mixed", AdaptUkrainianForFonix(u8"Pip-Boy працює"), "Pip-Boy prahtsyooyeh");
	failed += !ExpectEq("apostrophe", AdaptUkrainianForFonix(u8"з'їсти"), "zyeestih");
	failed += !ExpectEq("dzh", AdaptUkrainianForFonix(u8"джерело"), "jehrehloh");
	failed += !ExpectEq("dz", AdaptUkrainianForFonix(u8"дзвін"), "dzveen");
	failed += !ExpectEq("sogodni", AdaptUkrainianForFonix(u8"сьогодні"), "syohohdnee");
	failed += !ExpectEq("nyoho", AdaptUkrainianForFonix(u8"нього"), "nyohoh");
	failed += !ExpectEq("yoho", AdaptUkrainianForFonix(u8"його"), "yohoh");
	failed += !ExpectEq("khlib", AdaptUkrainianForFonix(u8"хліб"), "hleeb");
	failed += !ExpectEq("sil", AdaptUkrainianForFonix(u8"сіль"), "seel");
	failed += !ExpectEq(
		"prepare uk",
		PrepareDialogueText("Ukrainian", u8"[Сарказм] Привіт, мешканцю."),
		"Prihveet, mehshkahntsyoo.");
	failed += !ExpectEq(
		"usenglish passthrough",
		PrepareDialogueText("USEnglish", u8"привіт"),
		u8"привіт");
	failed += !ExpectEq("lang alias", ResolveFaceFxLanguage("Ukrainian"), "USEnglish");
	failed += !ExpectEq("lang passthrough", ResolveFaceFxLanguage("USEnglish"), "USEnglish");

	if (failed == 0)
		printf("all %s\n", "passed");
	else
		printf("%d failed\n", failed);

	return failed == 0 ? 0 : 1;
}
