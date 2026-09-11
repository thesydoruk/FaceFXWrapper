#include "Respell.h"
#include "RespellLang.h"
#include "RespellUtil.h"

namespace
{
	using respell::CollapseWs;
	using respell::DecodeUtf8;
	using respell::ExpectEq;
	using respell::TitleCase;

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

	int SelfTest()
	{
		int failed = 0;

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
		return failed;
	}
}

void RegisterUkrainianRespell()
{
	static const RespellLanguage language = {
		"Ukrainian",
		"uk",
		"USEnglish",
		u8"Привіт",
		AdaptUkrainianForFonix,
		SelfTest
	};
	RegisterRespellLanguage(language);
}
