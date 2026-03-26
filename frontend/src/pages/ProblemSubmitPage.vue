<template>
  <section class="page-grid single-column">
    <article class="panel submit-panel">
      <div class="submit-header">
        <RouterLink
          class="submit-back-link"
          :to="{ name: 'problem-detail', params: { problemId: numericProblemId } }"
        >
          문제로 돌아가기
        </RouterLink>
      </div>

      <div v-if="isLoadingProblem" class="empty-state">
        <p>문제 정보를 불러오는 중입니다.</p>
      </div>

      <div v-else-if="problemErrorMessage" class="empty-state error-state">
        <p>{{ problemErrorMessage }}</p>
      </div>

      <template v-else-if="problemDetail">
        <div class="panel-header submit-title-block">
          <div>
            <p class="panel-kicker">submit</p>
            <h3>#{{ problemDetail.problem_id }} {{ problemDetail.title }} 제출</h3>
          </div>
        </div>

        <article class="submit-form-panel">
          <div v-if="authState.isInitializing" class="empty-state">
            <p>로그인 상태를 확인하는 중입니다.</p>
          </div>

          <div v-else-if="!isAuthenticated" class="empty-state">
            <p>제출하려면 먼저 로그인하세요.</p>
          </div>

          <form v-else class="submission-form" @submit.prevent="submitSolution">
            <div class="submission-field">
              <label for="submission-language">언어</label>
              <select
                id="submission-language"
                v-model="selectedLanguage"
                class="submission-select"
                :disabled="isLoadingLanguages || !supportedLanguages.length || isSubmittingSubmission"
              >
                <option value="" disabled>
                  {{ isLoadingLanguages ? '언어 목록을 불러오는 중입니다.' : '언어를 선택하세요.' }}
                </option>
                <option
                  v-for="language in supportedLanguages"
                  :key="language.language"
                  :value="language.language"
                >
                  {{ language.language }} ({{ language.source_extension }})
                </option>
              </select>
            </div>

            <div class="submission-field">
              <label for="submission-source">소스 코드</label>
              <textarea
                id="submission-source"
                v-model="sourceCode"
                class="submission-editor"
                spellcheck="false"
              />
            </div>

            <p v-if="languageErrorMessage" class="submission-feedback is-error">
              {{ languageErrorMessage }}
            </p>
            <p v-else-if="submitErrorMessage" class="submission-feedback is-error">
              {{ submitErrorMessage }}
            </p>
            <p v-else-if="submissionSuccessMessage" class="submission-feedback is-success">
              {{ submissionSuccessMessage }}
            </p>

            <div class="submission-actions">
              <span
                v-if="activeLanguage"
                class="submission-language-hint"
              >
                확장자 {{ activeLanguage.source_extension }}
              </span>
              <button
                type="submit"
                class="primary-button"
                :disabled="!canSubmit"
              >
                {{ isSubmittingSubmission ? '제출 중...' : '제출' }}
              </button>
            </div>
          </form>
        </article>
      </template>
    </article>
  </section>
</template>

<script setup>
import { computed, onMounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { getSupportedLanguages } from '@/api/http'
import { getProblemDetail } from '@/api/problem'
import { createSubmission } from '@/api/submission'
import { useAuth } from '@/composables/useAuth'

const route = useRoute()
const router = useRouter()
const { authState, isAuthenticated, initializeAuth } = useAuth()
const numericProblemId = computed(() => Number.parseInt(route.params.problemId, 10))

const isLoadingProblem = ref(true)
const problemErrorMessage = ref('')
const problemDetail = ref(null)

const isLoadingLanguages = ref(true)
const languageErrorMessage = ref('')
const supportedLanguages = ref([])
const selectedLanguage = ref('')
const sourceCode = ref('')

const isSubmittingSubmission = ref(false)
const submitErrorMessage = ref('')
const submissionSuccessMessage = ref('')

const activeLanguage = computed(() =>
  supportedLanguages.value.find((language) => language.language === selectedLanguage.value) || null
)

const canSubmit = computed(() =>
  isAuthenticated.value &&
  Boolean(selectedLanguage.value) &&
  Boolean(sourceCode.value.trim()) &&
  !isSubmittingSubmission.value &&
  !isLoadingLanguages.value
)

watch(numericProblemId, () => {
  submitErrorMessage.value = ''
  submissionSuccessMessage.value = ''
  sourceCode.value = ''
  loadProblemDetail()
})

watch(supportedLanguages, (languages) => {
  if (!languages.length) {
    selectedLanguage.value = ''
    return
  }

  const hasSelectedLanguage = languages.some(
    (language) => language.language === selectedLanguage.value
  )

  if (!hasSelectedLanguage) {
    selectedLanguage.value = languages[0].language
  }
})

async function loadProblemDetail(){
  isLoadingProblem.value = true
  problemErrorMessage.value = ''

  if (!Number.isInteger(numericProblemId.value) || numericProblemId.value <= 0) {
    problemErrorMessage.value = '올바르지 않은 문제 번호입니다.'
    problemDetail.value = null
    isLoadingProblem.value = false
    return
  }

  try {
    const response = await getProblemDetail(numericProblemId.value)

    problemDetail.value = {
      ...response,
      limits: response.limits || {
        memory_limit_mb: 0,
        time_limit_ms: 0
      },
      statistics: {
        accepted_count: Number(response.statistics?.accepted_count ?? 0),
        submission_count: Number(response.statistics?.submission_count ?? 0)
      }
    }
  } catch (error) {
    problemErrorMessage.value = error instanceof Error
      ? error.message
      : '문제 정보를 불러오지 못했습니다.'
    problemDetail.value = null
  } finally {
    isLoadingProblem.value = false
  }
}

async function loadSupportedLanguageList(){
  isLoadingLanguages.value = true
  languageErrorMessage.value = ''

  try {
    const response = await getSupportedLanguages()
    supportedLanguages.value = Array.isArray(response.languages)
      ? response.languages
      : []
  } catch (error) {
    languageErrorMessage.value = error instanceof Error
      ? error.message
      : '지원 언어를 불러오지 못했습니다.'
    supportedLanguages.value = []
  } finally {
    isLoadingLanguages.value = false
  }
}

async function submitSolution(){
  if (!isAuthenticated.value || !authState.token) {
    submitErrorMessage.value = '제출하려면 로그인하세요.'
    submissionSuccessMessage.value = ''
    return
  }

  if (!selectedLanguage.value) {
    submitErrorMessage.value = '언어를 선택하세요.'
    submissionSuccessMessage.value = ''
    return
  }

  if (!sourceCode.value.trim()) {
    submitErrorMessage.value = '소스 코드를 입력하세요.'
    submissionSuccessMessage.value = ''
    return
  }

  isSubmittingSubmission.value = true
  submitErrorMessage.value = ''
  submissionSuccessMessage.value = ''

  try {
    await createSubmission(
      numericProblemId.value,
      {
        language: selectedLanguage.value,
        source_code: sourceCode.value
      },
      authState.token
    )

    await router.push({
      name: 'problem-my-submissions',
      params: {
        problemId: String(numericProblemId.value)
      }
    })
  } catch (error) {
    submitErrorMessage.value = error instanceof Error
      ? error.message
      : '제출을 처리하지 못했습니다.'
  } finally {
    isSubmittingSubmission.value = false
  }
}

onMounted(() => {
  initializeAuth()
  loadProblemDetail()
  loadSupportedLanguageList()
})
</script>

<style scoped>
.submit-panel {
  display: grid;
  gap: 1.25rem;
}

.submit-header {
  display: flex;
  justify-content: flex-start;
}

.submit-back-link {
  display: inline-flex;
  align-items: center;
  min-height: 2.6rem;
  padding: 0.6rem 1rem;
  border-radius: 999px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.72);
  font-weight: 700;
  transition:
    transform 160ms ease,
    background 160ms ease,
    border-color 160ms ease;
}

.submit-back-link:hover {
  transform: translateY(-1px);
  border-color: rgba(20, 33, 61, 0.24);
}

.submit-title-block {
  align-items: start;
}

.submit-metric-label {
  color: var(--ink-soft);
  font-size: 0.82rem;
  font-weight: 700;
  letter-spacing: 0.08em;
}

.submit-form-panel {
  border: 1px solid var(--line);
  border-radius: 20px;
  background: var(--surface-strong);
}

.submit-form-panel {
  padding: 1.2rem;
}

.submission-form {
  display: grid;
  gap: 1rem;
}

.submission-field {
  display: grid;
  gap: 0.5rem;
}

.submission-field label {
  font-weight: 700;
}

.submission-select,
.submission-editor {
  width: 100%;
  border-radius: 18px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.92);
  color: var(--ink-strong);
  font: inherit;
}

.submission-select {
  min-height: 3rem;
  padding: 0.75rem 0.95rem;
}

.submission-editor {
  min-height: 20rem;
  padding: 1rem;
  resize: vertical;
  font-family: "SFMono-Regular", "Consolas", monospace;
  line-height: 1.55;
}

.submission-select:focus,
.submission-editor:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}

.submission-feedback {
  margin: 0;
  padding: 0.9rem 1rem;
  border-radius: 16px;
  font-weight: 600;
}

.submission-feedback.is-error {
  color: var(--danger);
  background: var(--danger-soft);
}

.submission-feedback.is-success {
  color: var(--success);
  background: var(--success-soft);
}

.submission-actions {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: center;
  flex-wrap: wrap;
}

.submission-language-hint {
  color: var(--ink-soft);
  font-size: 0.92rem;
  font-weight: 600;
}

@media (max-width: 900px) {
  .submit-title-block {
    align-items: start;
  }

  .submission-actions {
    align-items: stretch;
  }
}
</style>
