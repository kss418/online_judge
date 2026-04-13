import { ref } from 'vue'

import { clearUserSubmissionBan, createUserSubmissionBan } from '@/api/userAdminApi'
import { noticeStore } from '@/stores/notice/noticeStore'
import { formatApiError } from '@/utils/apiError'

const durationPresets = Object.freeze([
  { label: '10분', minutes: 10 },
  { label: '1시간', minutes: 60 },
  { label: '1일', minutes: 1440 },
  { label: '7일', minutes: 10080 },
  { label: '30일', minutes: 43200 }
])

export function useAdminUserSubmissionBanActions({
  authState,
  canManageUsers,
  patchUser
}){
  const durationDrafts = ref({})
  const actionUserId = ref(0)
  const actionType = ref('')

  function getDurationDraft(userId){
    return durationDrafts.value[userId] ?? ''
  }

  function setDurationDraft(userId, durationMinutes){
    durationDrafts.value = {
      ...durationDrafts.value,
      [userId]: String(durationMinutes)
    }
  }

  function updateDurationDraft(userId, value){
    durationDrafts.value = {
      ...durationDrafts.value,
      [userId]: typeof value === 'string' ? value : ''
    }
  }

  function parseDurationDraft(userId){
    const rawValue = String(getDurationDraft(userId)).trim()
    const durationMinutes = Number(rawValue)

    if (!Number.isInteger(durationMinutes) || durationMinutes <= 0) {
      return 0
    }

    return durationMinutes
  }

  function isBusyUser(userId){
    return actionUserId.value === userId
  }

  async function handleCreateSubmissionBan(user){
    if (!authState.token || !canManageUsers.value) {
      noticeStore.showErrorNotice('제출 밴 설정은 관리자만 할 수 있습니다.', {
        duration: 5000
      })
      return
    }

    const durationMinutes = parseDurationDraft(user.user_id)

    if (!durationMinutes) {
      noticeStore.showErrorNotice('제출 밴 시간은 1분 이상의 정수로 입력해 주세요.', {
        duration: 5000
      })
      return
    }

    actionUserId.value = user.user_id
    actionType.value = 'create'

    try {
      const response = await createUserSubmissionBan(
        user.user_id,
        durationMinutes,
        authState.token
      )

      patchUser(user.user_id, {
        submission_banned_until: response.submission_banned_until,
        submission_banned_until_timestamp: response.submission_banned_until_timestamp,
        submission_banned_until_label: response.submission_banned_until_label,
        submission_ban_status_loading: false,
        submission_ban_status_error: false
      })
      durationDrafts.value = {
        ...durationDrafts.value,
        [user.user_id]: ''
      }
      noticeStore.showSuccessNotice(`${user.user_login_id} 님의 제출을 ${durationMinutes}분 동안 제한했습니다.`)
    } catch (error) {
      noticeStore.showErrorNotice(
        formatApiError(error, {
          fallback: '제출 밴을 설정하지 못했습니다.'
        }),
        { duration: 5000 }
      )
    } finally {
      actionUserId.value = 0
      actionType.value = ''
    }
  }

  async function handleClearSubmissionBan(user){
    if (!authState.token || !canManageUsers.value) {
      noticeStore.showErrorNotice('제출 밴 해제는 관리자만 할 수 있습니다.', {
        duration: 5000
      })
      return
    }

    actionUserId.value = user.user_id
    actionType.value = 'clear'

    try {
      const response = await clearUserSubmissionBan(user.user_id, authState.token)

      patchUser(user.user_id, {
        submission_banned_until: response.submission_banned_until,
        submission_banned_until_timestamp: response.submission_banned_until_timestamp,
        submission_banned_until_label: response.submission_banned_until_label,
        submission_ban_status_loading: false,
        submission_ban_status_error: false
      })
      noticeStore.showSuccessNotice(`${user.user_login_id} 님의 제출 제한을 해제했습니다.`)
    } catch (error) {
      noticeStore.showErrorNotice(
        formatApiError(error, {
          fallback: '제출 밴을 해제하지 못했습니다.'
        }),
        { duration: 5000 }
      )
    } finally {
      actionUserId.value = 0
      actionType.value = ''
    }
  }

  return {
    durationPresets,
    actionUserId,
    actionType,
    getDurationDraft,
    setDurationDraft,
    updateDurationDraft,
    isBusyUser,
    handleCreateSubmissionBan,
    handleClearSubmissionBan
  }
}
