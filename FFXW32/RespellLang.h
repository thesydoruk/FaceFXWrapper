#pragma once

#include <string>

// Plugin surface for one Fonix respell language.
// Add a language: new Foo.cpp that calls RegisterRespellLanguage, then list
// RegisterFooRespell() in RespellLanguages.cpp. Do not edit other languages.
struct RespellLanguage
{
	const char *name;        // "Ukrainian"
	const char *shortId;     // "uk" → test-uk
	const char *fonixName;   // "USEnglish"
	const char *exampleText; // CLI usage sample
	std::string (*adapt)(const std::string &sanitizedUtf8);
	int (*selfTest)();
};

void RegisterRespellLanguage(const RespellLanguage &Language);
void RegisterAllRespellLanguages();
