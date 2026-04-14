const numberFormatterCache = new Map()

function buildFormatterCacheKey(options = {}){
  const { locale, ...formatOptions } = options

  return JSON.stringify([
    locale ?? null,
    Object.keys(formatOptions)
      .sort()
      .map((key) => [key, formatOptions[key]])
  ])
}

function getNumberFormatter(options = {}){
  const cacheKey = buildFormatterCacheKey(options)

  if (!numberFormatterCache.has(cacheKey)) {
    const { locale, ...formatOptions } = options
    numberFormatterCache.set(
      cacheKey,
      locale == null
        ? new Intl.NumberFormat(undefined, formatOptions)
        : new Intl.NumberFormat(locale, formatOptions)
    )
  }

  return numberFormatterCache.get(cacheKey)
}

export function formatCount(value, options = {}){
  const numericValue = Number(value ?? 0)

  return getNumberFormatter(options).format(
    Number.isFinite(numericValue) ? numericValue : 0
  )
}

export function formatAcceptanceRate(acceptedCount, submissionCount, options = {}){
  const numericAcceptedCount = Number(acceptedCount)
  const numericSubmissionCount = Number(submissionCount)

  if (
    !Number.isFinite(numericAcceptedCount) ||
    !Number.isFinite(numericSubmissionCount) ||
    numericSubmissionCount <= 0
  ) {
    return '-'
  }

  const rate = (numericAcceptedCount / numericSubmissionCount) * 100

  if (!Number.isFinite(rate)) {
    return '-'
  }

  return `${getNumberFormatter({
    locale: 'ko-KR',
    minimumFractionDigits: 1,
    maximumFractionDigits: 1,
    ...options
  }).format(rate)}%`
}
