import { computed } from 'vue'

import { getMySubmissionBan } from '@/api/user'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { formatApiError } from '@/utils/apiError'
import { formatRemainingDuration, formatTimestamp } from '@/utils/dateTime'

function createInitialSubmissionBanState(){
  return {
    submission_banned_until: null,
    timestamp: null,
    label: ''
  }
}

export function useMySubmissionBanResource({
  authState,
  isOwnProfile,
  nowTimestamp
}){
  const submissionBanResource = useAsyncResource({
    initialData: createInitialSubmissionBanState,
    async load(token){
      const payload = await getMySubmissionBan(token)

      return {
        submission_banned_until: payload.submission_banned_until,
        timestamp: payload.submission_banned_until_timestamp,
        label: payload.submission_banned_until_label
      }
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
  const isMySubmissionBanActive = computed(() => (
    isOwnProfile.value &&
    typeof mySubmissionBan.value.timestamp === 'number' &&
    !Number.isNaN(mySubmissionBan.value.timestamp) &&
    mySubmissionBan.value.timestamp > nowTimestamp.value
  ))
  const mySubmissionBanStatusLabel = computed(() => {
    if (!isOwnProfile.value) {
      return ''
    }

    if (isMySubmissionBanLoading.value) {
      return '확인 중'
    }

    if (mySubmissionBanErrorMessage.value) {
      return '조회 실패'
    }

    if (isMySubmissionBanActive.value) {
      return '제출 금지'
    }

    return '정상'
  })
  const mySubmissionBanStatusTone = computed(() => {
    if (!isOwnProfile.value) {
      return 'neutral'
    }

    if (isMySubmissionBanLoading.value) {
      return 'neutral'
    }

    if (mySubmissionBanErrorMessage.value) {
      return 'warning'
    }

    return isMySubmissionBanActive.value ? 'danger' : 'success'
  })
  const mySubmissionBanWindowText = computed(() => {
    if (!isOwnProfile.value) {
      return ''
    }

    if (isMySubmissionBanLoading.value) {
      return '제출 제한 상태를 확인하는 중입니다.'
    }

    if (mySubmissionBanErrorMessage.value) {
      return mySubmissionBanErrorMessage.value
    }

    if (!mySubmissionBan.value.submission_banned_until) {
      return '현재 제출 제한이 없습니다.'
    }

    if (isMySubmissionBanActive.value) {
      return `${formatRemainingDuration(nowTimestamp.value, mySubmissionBan.value.timestamp)} 남음`
    }

    return '이전 제출 제한이 만료되었습니다.'
  })
  const mySubmissionBanUntilText = computed(() => {
    if (
      !isOwnProfile.value ||
      typeof mySubmissionBan.value.submission_banned_until !== 'string' ||
      !mySubmissionBan.value.submission_banned_until
    ) {
      return '-'
    }

    return formatTimestamp(mySubmissionBan.value.submission_banned_until)
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
