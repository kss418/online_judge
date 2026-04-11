import { requestJson, apiBaseUrl } from '@/api/http'
import {
  normalizeArray,
  normalizeString,
  normalizeTrimmedString
} from '@/api/normalizers/common'

function normalizeSystemHealthResponse(response){
  return {
    message: normalizeTrimmedString(response?.message, 'ok')
  }
}

function normalizeSupportedLanguage(language){
  return {
    language: normalizeTrimmedString(language?.language),
    source_extension: normalizeString(language?.source_extension)
  }
}

function normalizeSupportedLanguagesResponse(response){
  return {
    languages: normalizeArray(response?.languages)
      .map(normalizeSupportedLanguage)
      .filter((language) => language.language)
  }
}

export function getSystemHealth(){
  return requestJson('/system/health').then(normalizeSystemHealthResponse)
}

export function getSupportedLanguages(){
  return requestJson('/system/supported-languages').then(normalizeSupportedLanguagesResponse)
}

export { apiBaseUrl }
