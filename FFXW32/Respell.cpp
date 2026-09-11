#include "Respell.h"
#include "RespellLang.h"
#include "RespellUtil.h"

#include <cstring>
#include <vector>

namespace
{
	std::vector<RespellLanguage> &Table()
	{
		static std::vector<RespellLanguage> table;
		return table;
	}

	void EnsureRegistered()
	{
		static bool done = false;
		if (done)
			return;
		done = true;
		RegisterAllRespellLanguages();
	}

	bool Matches(const RespellLanguage &Lang, const char *Key)
	{
		return Key && (_stricmp(Lang.name, Key) == 0 || _stricmp(Lang.shortId, Key) == 0);
	}

	const RespellLanguage *Find(const char *Key)
	{
		EnsureRegistered();
		if (!Key)
			return nullptr;
		for (const auto &lang : Table())
		{
			if (Matches(lang, Key))
				return &lang;
		}
		return nullptr;
	}

	int RunOne(const RespellLanguage &Lang)
	{
		if (!Lang.selfTest)
			return 0;
		printf("FaceFXWrapper %s self-test (Fonix respell)\n", Lang.name);
		const int failed = Lang.selfTest();
		if (failed == 0)
			printf("all %s\n", "passed");
		else
			printf("%d failed\n", failed);
		return failed == 0 ? 0 : 1;
	}
}

void RegisterRespellLanguage(const RespellLanguage &Language)
{
	if (!Language.name || !Language.adapt)
		return;
	for (const auto &existing : Table())
	{
		if (_stricmp(existing.name, Language.name) == 0)
			return;
	}
	Table().push_back(Language);
}

bool IsRespellLanguage(const char *Language)
{
	return Find(Language) != nullptr;
}

std::string PrepareDialogueText(const char *Language, const char *Text)
{
	const auto *lang = Find(Language);
	if (!lang)
		return Text ? Text : "";
	return lang->adapt(respell::SanitizeDialogueText(Text));
}

const char *ResolveFaceFxLanguage(const char *Language)
{
	if (const auto *lang = Find(Language))
		return lang->fonixName ? lang->fonixName : "USEnglish";
	return Language;
}

int RunRespellSelfTest(const char *IdOrName)
{
	EnsureRegistered();

	if (!IdOrName || !IdOrName[0] || _stricmp(IdOrName, "all") == 0)
	{
		int failed = 0;
		for (const auto &lang : Table())
			failed += RunOne(lang);
		return failed == 0 ? 0 : 1;
	}

	if (const auto *lang = Find(IdOrName))
		return RunOne(*lang);

	printf("Unknown respell test \"%s\". Try test-all", IdOrName);
	for (const auto &lang : Table())
		printf(", test-%s", lang.shortId);
	printf(".\n");
	return 1;
}

void PrintRespellUsage()
{
	EnsureRegistered();

	printf("Lang is Fonix \"USEnglish\", or a respell language (strip [tags], then USEnglish):\n");
	for (const auto &lang : Table())
		printf("\t%s (test-%s)\n", lang.name, lang.shortId);
	printf("Respell runs only for those Lang values. serve text is UTF-8.\n");
	printf("\n");
	printf("Examples:\n");
	printf("\tFaceFXWrapper \"Skyrim\" \"USEnglish\" \"C:\\FonixData.cdf\" \"C:\\input.wav\" \"C:\\input_resampled.wav\" \"C:\\output.lip\" \"Blah Blah Blah\"\n");
	for (const auto &lang : Table())
	{
		printf("\tFaceFXWrapper \"Fallout4\" \"%s\" \"C:\\FonixData.cdf\" \"C:\\input_resampled.wav\" \"C:\\output.lip\" \"%s\"\n",
			lang.name,
			lang.exampleText ? lang.exampleText : "");
	}
	printf("\tFaceFXWrapper serve Fallout4\n");
	printf("\tFaceFXWrapper test-all\n");
	for (const auto &lang : Table())
		printf("\tFaceFXWrapper test-%s\n", lang.shortId);
}
