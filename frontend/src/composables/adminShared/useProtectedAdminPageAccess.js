import { computed, onMounted, watch } from 'vue'

export function useProtectedAdminPageAccess({
  authState,
  initializeAuth,
  isAuthenticated,
  hasAccess,
  onDenied,
  onAllowed,
  loggedOutMessage,
  deniedMessage
}){
  const accessState = computed(() => {
    if (!authState.initialized || authState.isInitializing) {
      return 'initializing'
    }

    if (!isAuthenticated.value) {
      return 'logged-out'
    }

    if (!hasAccess.value) {
      return 'denied'
    }

    return 'allowed'
  })

  const accessMessage = computed(() => {
    if (accessState.value === 'initializing') {
      return '관리자 권한을 확인하는 중입니다.'
    }

    if (accessState.value === 'logged-out') {
      return loggedOutMessage
    }

    if (accessState.value === 'denied') {
      return deniedMessage
    }

    return ''
  })

  const canAccessPage = computed(() => accessState.value === 'allowed')

  watch(accessState, (nextState, previousState) => {
    if (nextState === 'initializing') {
      return
    }

    if (nextState === 'allowed') {
      if (previousState !== 'allowed' && typeof onAllowed === 'function') {
        void onAllowed()
      }
      return
    }

    if (nextState !== previousState && typeof onDenied === 'function') {
      void onDenied()
    }
  }, {
    immediate: true
  })

  function watchWhenAllowed(source, callback, options = {}){
    return watch(source, (...args) => {
      if (!canAccessPage.value) {
        return
      }

      callback(...args)
    }, options)
  }

  onMounted(() => {
    void initializeAuth()
  })

  return {
    accessState,
    accessMessage,
    canAccessPage,
    watchWhenAllowed
  }
}
