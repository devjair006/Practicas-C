#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <string>

enum Language { LANG_ES, LANG_EN };
extern Language currentLanguage;

const char* getText(const std::string& key);

#endif // LOCALIZATION_H
