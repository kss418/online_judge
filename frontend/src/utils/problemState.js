import { normalizeProblemState } from '@/api/normalizers/problemState'

const problemStatePresentationMap = {
  solved: {
    label: '성공',
    tone: 'success',
    textClass: 'problem-state-text--solved'
  },
  wrong: {
    label: '실패',
    tone: 'danger',
    textClass: 'problem-state-text--wrong'
  }
}

export function getProblemStateLabel(problemState){
  return problemStatePresentationMap[normalizeProblemState(problemState)]?.label || ''
}

export function getProblemStateTone(problemState){
  return problemStatePresentationMap[normalizeProblemState(problemState)]?.tone || 'neutral'
}

export function getProblemStateTextClass(problemState){
  return problemStatePresentationMap[normalizeProblemState(problemState)]?.textClass || ''
}
