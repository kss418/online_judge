export function formatTimestamp(value){
  if (typeof value !== 'string' || !value.trim()) {
    return '-'
  }

  const trimmedValue = value.trim()
  const directMatch = trimmedValue.match(/^(\d{4}-\d{2}-\d{2})[ T](\d{2}:\d{2})/)
  if (directMatch) {
    return `${directMatch[1]} ${directMatch[2]}`
  }

  const parsedDate = new Date(trimmedValue)
  if (Number.isNaN(parsedDate.getTime())) {
    return trimmedValue
  }

  const year = String(parsedDate.getFullYear())
  const month = String(parsedDate.getMonth() + 1).padStart(2, '0')
  const day = String(parsedDate.getDate()).padStart(2, '0')
  const hours = String(parsedDate.getHours()).padStart(2, '0')
  const minutes = String(parsedDate.getMinutes()).padStart(2, '0')
  return `${year}-${month}-${day} ${hours}:${minutes}`
}

export function formatRelativeTimestamp(nowTimestamp, timestamp, fallback = '-'){
  if (typeof timestamp !== 'number' || Number.isNaN(timestamp)) {
    return fallback
  }

  const elapsedSeconds = Math.max(1, Math.floor((nowTimestamp - timestamp) / 1000))

  if (elapsedSeconds < 60) {
    return `${elapsedSeconds}초 전`
  }

  const elapsedMinutes = Math.floor(elapsedSeconds / 60)
  if (elapsedMinutes < 60) {
    return `${elapsedMinutes}분 전`
  }

  const elapsedHours = Math.floor(elapsedMinutes / 60)
  if (elapsedHours < 24) {
    return `${elapsedHours}시간 전`
  }

  const elapsedDays = Math.floor(elapsedHours / 24)
  if (elapsedDays < 30) {
    return `${elapsedDays}일 전`
  }

  const elapsedMonths = Math.floor(elapsedDays / 30)
  if (elapsedMonths < 12) {
    return `${elapsedMonths}달 전`
  }

  return `${Math.floor(elapsedDays / 365)}년 전`
}

export function formatRemainingDuration(nowTimestamp, timestamp){
  if (typeof timestamp !== 'number' || Number.isNaN(timestamp)) {
    return '-'
  }

  const remainingSeconds = Math.max(1, Math.floor((timestamp - nowTimestamp) / 1000))
  if (remainingSeconds < 60) {
    return `${remainingSeconds}초`
  }

  const remainingMinutes = Math.floor(remainingSeconds / 60)
  if (remainingMinutes < 60) {
    return `${remainingMinutes}분`
  }

  const remainingHours = Math.floor(remainingMinutes / 60)
  if (remainingHours < 24) {
    return `${remainingHours}시간`
  }

  const remainingDays = Math.floor(remainingHours / 24)
  if (remainingDays < 30) {
    return `${remainingDays}일`
  }

  const remainingMonths = Math.floor(remainingDays / 30)
  if (remainingMonths < 12) {
    return `${remainingMonths}달`
  }

  return `${Math.floor(remainingDays / 365)}년`
}
