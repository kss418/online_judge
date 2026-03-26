import { requestJson } from '@/api/http'
import {
  normalizeProblemStateRecord,
  normalizeProblemStateRecords
} from '@/utils/problemState'

export async function getProblemList(options = {}){
  const { bearerToken = '', title = '' } = options
  const searchParams = new URLSearchParams()

  if (title?.trim()) {
    searchParams.set('title', title.trim())
  }

  const queryString = searchParams.toString()
  const path = queryString ? `/problem?${queryString}` : '/problem'

  const response = await requestJson(path, {
    bearerToken
  })

  return {
    ...response,
    problems: normalizeProblemStateRecords(response.problems)
  }
}

export async function getProblemDetail(problemId, options = {}){
  const { bearerToken = '' } = options

  const response = await requestJson(`/problem/${problemId}`, {
    bearerToken
  })

  return normalizeProblemStateRecord(response)
}
