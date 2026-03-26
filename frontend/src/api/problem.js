import { requestJson } from '@/api/http'

export function getProblemList(options = {}){
  const { bearerToken = '', title = '' } = options
  const searchParams = new URLSearchParams()

  if (title?.trim()) {
    searchParams.set('title', title.trim())
  }

  const queryString = searchParams.toString()
  const path = queryString ? `/problem?${queryString}` : '/problem'

  return requestJson(path, {
    bearerToken
  })
}

export function getProblemDetail(problemId, options = {}){
  const { bearerToken = '' } = options

  return requestJson(`/problem/${problemId}`, {
    bearerToken
  })
}
