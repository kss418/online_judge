import { requestJson } from '@/api/http'

export function getProblemList(options = {}){
  const searchParams = new URLSearchParams()

  if (options.title?.trim()) {
    searchParams.set('title', options.title.trim())
  }

  const queryString = searchParams.toString()
  const path = queryString ? `/problem?${queryString}` : '/problem'

  return requestJson(path)
}
