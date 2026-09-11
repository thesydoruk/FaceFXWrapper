#pragma once

#include <string>

bool IsUkrainianLanguage(const char *Language);

// Only when Lang is Ukrainian: strip [tone tags], respell Cyrillic for Fonix.
// Other languages return the text unchanged. Input is UTF-8.
std::string PrepareDialogueText(const char *Language, const char *Text);

// "Ukrainian" (any case) is an alias for Fonix "USEnglish".
const char *ResolveFaceFxLanguage(const char *Language);

// Compare the C++ table to the transynth ukToFonix fixtures. Prints to stdout.
int RunUkrainianSelfTest();
