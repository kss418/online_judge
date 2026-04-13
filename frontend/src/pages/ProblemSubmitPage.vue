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
              <div
                class="submission-editor-shell"
                :class="{ 'is-readonly': isSubmittingSubmission }"
              >
                <div class="submission-editor-gutter" aria-hidden="true">
                  <span
                    v-for="lineNumber in editorLineNumbers"
                    :key="lineNumber"
                    class="submission-editor-line-number"
                  >
                    {{ lineNumber }}
                  </span>
                </div>
                <textarea
                  id="submission-source"
                  ref="sourceEditorElement"
                  v-model="sourceCode"
                  class="submission-editor-input"
                  spellcheck="false"
                  wrap="off"
                  :readonly="isSubmittingSubmission"
                  :placeholder="editorPlaceholder"
                  @input="handleSourceEditorInput"
                  @keydown="handleEditorKeydown"
                />
              </div>
            </div>

            <p v-if="isSubmissionBanActive" class="submission-feedback is-warning">
              제출이 {{ submissionBanUntilText }}까지 제한되어 있습니다.
              <span v-if="submissionBanRemainingText">
                {{ submissionBanRemainingText }}
              </span>
            </p>
            <p v-else-if="languageErrorMessage" class="submission-feedback is-error">
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
import { useProblemSubmitPage } from '@/composables/useProblemSubmitPage'

const {
  authState,
  isAuthenticated,
  numericProblemId,
  isLoadingProblem,
  problemErrorMessage,
  problemDetail,
  isLoadingLanguages,
  languageErrorMessage,
  supportedLanguages,
  isSubmissionBanActive,
  submissionBanUntilText,
  submissionBanRemainingText,
  selectedLanguage,
  sourceCode,
  sourceEditorElement,
  activeLanguage,
  editorLineNumbers,
  editorPlaceholder,
  isSubmittingSubmission,
  submitErrorMessage,
  submissionSuccessMessage,
  canSubmit,
  handleSourceEditorInput,
  handleEditorKeydown,
  submitSolution
} = useProblemSubmitPage()
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
.submission-editor-input {
  width: 100%;
  color: var(--ink-strong);
  font: inherit;
}

.submission-select {
  min-height: 3rem;
  padding: 0.75rem 0.95rem;
}

.submission-editor-shell {
  display: grid;
  grid-template-columns: auto minmax(0, 1fr);
  border-radius: 20px;
  border: 1px solid rgba(20, 33, 61, 0.14);
  background:
    linear-gradient(90deg, rgba(148, 163, 184, 0.12) 0, rgba(148, 163, 184, 0.12) 4.4rem, rgba(255, 255, 255, 0.96) 4.4rem, rgba(255, 255, 255, 0.96) 100%);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.75);
  overflow: hidden;
}

.submission-editor-shell.is-readonly {
  opacity: 0.84;
}

.submission-editor-gutter {
  display: grid;
  align-content: start;
  min-width: 4.4rem;
  padding: 1rem 0.7rem 1rem 0.95rem;
  border-right: 1px solid rgba(20, 33, 61, 0.08);
  background: rgba(148, 163, 184, 0.06);
  color: rgba(71, 85, 105, 0.88);
  font-family: "IBM Plex Mono", "SFMono-Regular", "Consolas", monospace;
  font-size: 0.92rem;
  line-height: 1.7;
  text-align: right;
  user-select: none;
}

.submission-editor-line-number {
  display: block;
}

.submission-editor-input {
  min-height: 20rem;
  padding: 1rem 1rem 1rem 1.15rem;
  border: 0;
  background: transparent;
  resize: none;
  overflow-x: auto;
  overflow-y: hidden;
  white-space: pre;
  font-family: "IBM Plex Mono", "SFMono-Regular", "Consolas", monospace;
  font-size: 0.96rem;
  line-height: 1.7;
  tab-size: 4;
}

.submission-editor-input::placeholder {
  color: rgba(71, 85, 105, 0.72);
}

.submission-select:focus,
.submission-editor-input:focus {
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

.submission-feedback.is-warning {
  color: var(--warning);
  background: var(--warning-soft);
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

  .submission-editor-shell {
    grid-template-columns: 3.5rem minmax(0, 1fr);
    background:
      linear-gradient(90deg, rgba(148, 163, 184, 0.12) 0, rgba(148, 163, 184, 0.12) 3.5rem, rgba(255, 255, 255, 0.96) 3.5rem, rgba(255, 255, 255, 0.96) 100%);
  }

  .submission-editor-gutter {
    min-width: 3.5rem;
    padding-left: 0.7rem;
    padding-right: 0.55rem;
    font-size: 0.84rem;
  }

  .submission-editor-input {
    padding-left: 0.85rem;
    font-size: 0.9rem;
  }
}
</style>
