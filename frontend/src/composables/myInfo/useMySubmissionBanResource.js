import { computed } from 'vue'

import { getMySubmissionBan } from '@/api/userQueryApi'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { formatApiError } from '@/utils/apiError'
import {
  createSubmissionBanState,
  formatSubmissionBanRemaining,
  formatSubmissionBanUntil,
  getSubmissionBanPhase,
  isSubmissionBanActive
} from '@/utils/submissionBan'

export function useMySubmissionBanResource({
  authState,
  isOwnProfile,
  nowTimestamp
}){
  const submissionBanResource = useAsyncResource({
    initialData: createSubmissionBanState,
    async load(token){
      return getMySubmissionBan(token)
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '제출 제한 상태를 불러오지 못했습니다.'
      })
    }
  })

  const mySubmissionBan = submissionBanResource.data
  const isMySubmissionBanLoading = submissionBanResource.isLoading
  const mySubmissionBanErrorMessage = submissionBanResource.errorMessage
  const mySubmissionBanPhase = computed(() => {
    if (!isOwnProfile.value) {
      return 'none'
    }

    return getSubmissionBanPhase({
      nowTimestamp: nowTimestamp.value,
      submissionBanTimestamp: mySubmissionBan.value.submission_banned_until_timestamp,
      isLoading: isMySubmissionBanLoading.value,
      hasError: Boolean(mySubmissionBanErrorMessage.value)
    })
  })
  const isMySubmissionBanActive = computed(() => (
    isOwnProfile.value &&
    isSubmissionBanActive(
      nowTimestamp.value,
      mySubmissionBan.value.submission_banned_until_timestamp
    )
  ))
  const mySubmissionBanStatusLabel = computed(() => {
    if (!isOwnProfile.value) {
      return ''
    }

    if (mySubmissionBanPhase.value === 'loading') {
      return '확인 중'
    }

    if (mySubmissionBanPhase.value === 'error') {
      return '조회 실패'
    }

    if (mySubmissionBanPhase.value === 'active') {
      return '제출 금지'
    }

    return '정상'
  })
  const mySubmissionBanStatusTone = computed(() => {
    if (!isOwnProfile.value) {
      return 'neutral'
    }

    if (mySubmissionBanPhase.value === 'loading') {
      return 'neutral'
    }

    if (mySubmissionBanPhase.value === 'error') {
      return 'warning'
    }

    return mySubmissionBanPhase.value === 'active' ? 'danger' : 'success'
  })
  const mySubmissionBanWindowText = computed(() => {
    if (!isOwnProfile.value) {
      return ''
    }

    if (mySubmissionBanPhase.value === 'loading') {
      return '제출 제한 상태를 확인하는 중입니다.'
    }

    if (mySubmissionBanPhase.value === 'error') {
      return mySubmissionBanErrorMessage.value
    }

    if (!mySubmissionBan.value.submission_banned_until) {
      return '현재 제출 제한이 없습니다.'
    }

    if (mySubmissionBanPhase.value === 'active') {
      return `${formatSubmissionBanRemaining(
        nowTimestamp.value,
        mySubmissionBan.value.submission_banned_until_timestamp
      )} 남음`
    }

    return '이전 제출 제한이 만료되었습니다.'
  })
  const mySubmissionBanUntilText = computed(() => {
    if (!isOwnProfile.value) {
      return '-'
    }

    return formatSubmissionBanUntil(mySubmissionBan.value.submission_banned_until)
  })

  function resetMySubmissionBan(){
    submissionBanResource.reset({
      preserveHasLoadedOnce: true,
      clearLastArgs: true
    })
  }

  async function loadMySubmissionBan(){
    if (!isOwnProfile.value || !authState.token) {
      resetMySubmissionBan()

      return {
        status: 'reset',
        data: mySubmissionBan.value
      }
    }

    return submissionBanResource.run(authState.token, {
      resetDataOnError: true
    })
  }

  return {
    mySubmissionBan,
    isMySubmissionBanLoading,
    mySubmissionBanErrorMessage,
    isMySubmissionBanActive,
    mySubmissionBanStatusLabel,
    mySubmissionBanStatusTone,
    mySubmissionBanWindowText,
    mySubmissionBanUntilText,
    resetMySubmissionBan,
    loadMySubmissionBan
  }
}
