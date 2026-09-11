// Catalog of respell plugins. Adding a language:
//  1. Create Foo.cpp with void RegisterFooRespell()
//  2. Declare and call it here
//  3. Add Foo.cpp to the vcxproj
// Do not edit Ukrainian.cpp / Polish.cpp / other language modules.

void RegisterUkrainianRespell();
void RegisterPolishRespell();

void RegisterAllRespellLanguages()
{
	RegisterUkrainianRespell();
	RegisterPolishRespell();
}
