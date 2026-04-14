import { onMounted, watch } from 'vue'

export function useProtectedAdminBootstrap({
  authState,
  initializeAuth,
  isAuthenticated,
  hasAccess,
  onDenied,
  onAllowed
}){
  watch(
    [
      () => authState.initialized,
      () => authState.token,
      isAuthenticated,
      hasAccess
    ],
    ([initialized, _token, authenticated, accessGranted]) => {
      if (!initialized) {
        return
      }

      if (!authenticated || !accessGranted) {
        if (typeof onDenied === 'function') {
          void onDenied()
        }
        return
      }

      if (typeof onAllowed === 'function') {
        void onAllowed()
      }
    },
    {
      immediate: true
    }
  )

  onMounted(() => {
    void initializeAuth()
  })
}
