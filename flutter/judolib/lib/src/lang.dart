import 'package:flutter/material.dart';

const languages = [
  const Locale('fi', ''),
  const Locale('sv', ''),
  const Locale('en', ''),
  const Locale('es', ''),
  const Locale('et', ''),
  const Locale('uk', ''),
  const Locale('is', ''),
  const Locale('nb', ''),
  const Locale('pl', ''),
  const Locale('sk', ''),
  const Locale('nl', ''),
  const Locale('cs', ''),
  const Locale('de', ''),
  const Locale('da', ''),
  const Locale('he', ''),
  const Locale('fr', ''),
  const Locale('fa', ''),
];

const languageCodes = ["fi", "sv", "en", "es", "et", "uk", "is", "nb", "pl", "sk", "nl", "cs", "de", "da", "he", "fr", "fa"];

const languageCodeToLanguage = {"fi": "Suomi", "sv": "Svensk", "en": "English", "es": "Español", "et": "Eesti",
  "uk": "Українська", "is": "Íslenska", "nb": "Norsk", "pl": "Polski", "sk": "Slovenčina",
  "nl": "Nederlands", "cs": "Čeština", "de": "Deutsch", "da": "Dansk", "he": "עברית", "fr": "Français", "fa": "فارسی"};

const languageCodeToIOC = {"fi": "FIN", "sv": "SWE", "en": "GBR", "es": "ESP", "et": "EST",
  "uk": "UKR", "is": "ISL", "nb": "NOR", "pl": "POL", "sk": "SLO",
  "nl": "NED", "cs": "CZE", "de": "GER", "da": "DEN", "he": "ISR", "fr": "FRA", "fa": "IRI"};

String languageCode = 'en';
String countryCode = '';

