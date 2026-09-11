#pragma once

#include <string>

bool IsRespellLanguage(const char *Language);

// Strip [tone tags] and respell when Lang is a registered respell language.
// Other languages return the text unchanged. Input is UTF-8.
std::string PrepareDialogueText(const char *Language, const char *Text);

// Respell languages alias to their Fonix name (usually "USEnglish").
const char *ResolveFaceFxLanguage(const char *Language);

// "uk", "Ukrainian", "all". Prints to stdout. 0 = pass.
int RunRespellSelfTest(const char *IdOrName);

void PrintRespellUsage();
