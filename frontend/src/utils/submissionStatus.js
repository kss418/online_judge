export const submissionStatusLabelMap = {
  queued: '대기 중',
  judging: '채점 중',
  accepted: '정답',
  wrong_answer: '오답',
  time_limit_exceeded: '시간 초과',
  memory_limit_exceeded: '메모리 초과',
  runtime_error: '런타임 에러',
  output_exceeded: '출력 초과',
  compile_error: '컴파일 에러'
}

export function getSubmissionStatusLabel(status){
  if (typeof status !== 'string') {
    return ''
  }

  return submissionStatusLabelMap[status] || status
}
