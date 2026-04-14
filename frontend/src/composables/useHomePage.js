import { computed, onMounted } from 'vue'

import { apiBaseUrl, getSupportedLanguages, getSystemHealth } from '@/api/system'
import { useAsyncResource } from '@/composables/useAsyncResource'
import { formatApiError } from '@/utils/apiError'

function createInitialOverviewState(){
  return {
    healthMessage: 'unknown',
    languages: []
  }
}

export function useHomePage(){
  const overviewResource = useAsyncResource({
    initialData: createInitialOverviewState,
    async load(){
      const [healthResponse, languageResponse] = await Promise.all([
        getSystemHealth(),
        getSupportedLanguages()
      ])

      return {
        healthMessage: healthResponse.message || 'ok',
        languages: Array.isArray(languageResponse.languages)
          ? languageResponse.languages
          : []
      }
    },
    getErrorMessage(error){
      return formatApiError(error, {
        fallback: '서비스와 연결할 수 없습니다.'
      })
    }
  })
  const isLoading = computed(() =>
    !overviewResource.hasLoadedOnce.value || overviewResource.isLoading.value
  )
  const healthMessage = computed(() => overviewResource.data.value.healthMessage)
  const languages = computed(() => overviewResource.data.value.languages)
  const statusLabel = computed(() => {
    if (isLoading.value) {
      return 'Loading'
    }

    return overviewResource.errorMessage.value ? 'Offline' : 'Connected'
  })
  const statusTone = computed(() => {
    if (isLoading.value) {
      return 'neutral'
    }

    return overviewResource.errorMessage.value ? 'danger' : 'success'
  })

  function loadOverview(){
    return overviewResource.run(undefined, {
      resetDataOnError: true
    })
  }

  onMounted(() => {
    void loadOverview()
  })

  return {
    apiBaseUrl,
    isLoading,
    errorMessage: overviewResource.errorMessage,
    healthMessage,
    languages,
    statusLabel,
    statusTone
  }
}
