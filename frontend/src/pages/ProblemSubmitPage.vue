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
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'

import { getSupportedLanguages } from '@/api/http'
import { getProblemDetail } from '@/api/problem'
import { createSubmission } from '@/api/submission'
import { getMySubmissionBan } from '@/api/user'
import { useAuth } from '@/composables/useAuth'
import { formatApiError } from '@/utils/apiError'

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
const sourceEditorElement = ref(null)

const isSubmittingSubmission = ref(false)
const submitErrorMessage = ref('')
const submissionSuccessMessage = ref('')
const isLoadingSubmissionBan = ref(false)
const submissionBan = ref({
  submission_banned_until: null,
  timestamp: null,
  label: ''
})
const nowTimestamp = ref(Date.now())
let submissionBanRefreshTimer = null
const submissionFieldLabels = {
  language: '언어',
  source_code: '소스 코드'
}

const activeLanguage = computed(() =>
  supportedLanguages.value.find((language) => language.language === selectedLanguage.value) || null
)

const editorLineCount = computed(() =>
  Math.max(1, sourceCode.value.split('\n').length)
)

const editorLineNumbers = computed(() =>
  Array.from({ length: editorLineCount.value }, (_, index) => index + 1)
)

const editorPlaceholder = computed(() => {
  if (!activeLanguage.value) {
    return '// 언어를 선택하면 예시 형식이 바뀝니다.'
  }

  const languageName = String(activeLanguage.value.language || '').toLowerCase()
  const sourceExtension = String(activeLanguage.value.source_extension || '').toLowerCase()

  if (languageName.includes('cpp') || sourceExtension === '.cpp') {
    return [
      '#include <bits/stdc++.h>',
      '',
      'using namespace std;',
      '',
      'int main(){',
      '    ios::sync_with_stdio(false);',
      '    cin.tie(nullptr);',
      '',
      '    return 0;',
      '}'
    ].join('\n')
  }

  if (languageName.includes('python') || sourceExtension === '.py') {
    return [
      'import sys',
      '',
      'def main():',
      '    input = sys.stdin.readline',
      '',
      '',
      "if __name__ == '__main__':",
      '    main()'
    ].join('\n')
  }

  if (languageName.includes('java') || sourceExtension === '.java') {
    return [
      'import java.io.*;',
      '',
      'public class Main {',
      '    public static void main(String[] args) throws Exception {',
      '    }',
      '}'
    ].join('\n')
  }

  return '// 여기에 코드를 작성하세요.'
})

const canSubmit = computed(() =>
  isAuthenticated.value &&
  Boolean(selectedLanguage.value) &&
  Boolean(sourceCode.value.trim()) &&
  !isSubmittingSubmission.value &&
  !isLoadingLanguages.value &&
  !isSubmissionBanActive.value
)

const isSubmissionBanActive = computed(() => (
  typeof submissionBan.value.timestamp === 'number' &&
  !Number.isNaN(submissionBan.value.timestamp) &&
  submissionBan.value.timestamp > nowTimestamp.value
))

const submissionBanUntilText = computed(() => {
  if (
    typeof submissionBan.value.submission_banned_until !== 'string' ||
    !submissionBan.value.submission_banned_until
  ) {
    return '-'
  }

  return formatTimestamp(submissionBan.value.submission_banned_until)
})

const submissionBanRemainingText = computed(() => {
  if (!isSubmissionBanActive.value) {
    return ''
  }

  return `약 ${formatRemainingDuration(submissionBan.value.timestamp)} 남음`
})

watch(numericProblemId, () => {
  submitErrorMessage.value = ''
  submissionSuccessMessage.value = ''
  sourceCode.value = ''
  loadProblemDetail()
})

watch(sourceCode, () => {
  scheduleEditorResize()
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

watch(
  () => [authState.initialized, authState.token, isAuthenticated.value],
  ([initialized, token, authenticated]) => {
    if (!initialized) {
      isLoadingSubmissionBan.value = true
      return
    }

    if (!authenticated || !token) {
      submissionBan.value = {
        submission_banned_until: null,
        timestamp: null,
        label: ''
      }
      isLoadingSubmissionBan.value = false
      return
    }

    void loadMySubmissionBan()
  },
  { immediate: true }
)

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
    problemErrorMessage.value = formatApiError(error, {
      fallback: '문제 정보를 불러오지 못했습니다.'
    })
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
    languageErrorMessage.value = formatApiError(error, {
      fallback: '지원 언어를 불러오지 못했습니다.'
    })
    supportedLanguages.value = []
  } finally {
    isLoadingLanguages.value = false
  }
}

function normalizeDateTime(value){
  if (typeof value !== 'string' || !value.trim()) {
    return {
      timestamp: null,
      label: ''
    }
  }

  const trimmedValue = value.trim()
  const matchedTimestamp = trimmedValue.match(
    /^(\d{4}-\d{2}-\d{2})[ T](\d{2}:\d{2}:\d{2})(?:\.(\d{1,6}))?([+-]\d{2})(?::?(\d{2}))?$/
  )

  if (matchedTimestamp) {
    const [, datePart, timePart, fractionPart = '', offsetHour, offsetMinute = '00'] =
      matchedTimestamp
    const normalizedFraction = fractionPart
      ? `.${fractionPart.slice(0, 3).padEnd(3, '0')}`
      : ''
    const parsedTimestamp = Date.parse(
      `${datePart}T${timePart}${normalizedFraction}${offsetHour}:${offsetMinute}`
    )

    return {
      timestamp: Number.isNaN(parsedTimestamp) ? null : parsedTimestamp,
      label: `${datePart} ${timePart}`
    }
  }

  const parsedTimestamp = Date.parse(trimmedValue.replace(' ', 'T'))
  return {
    timestamp: Number.isNaN(parsedTimestamp) ? null : parsedTimestamp,
    label: trimmedValue
  }
}

function formatTimestamp(value){
  if (typeof value !== 'string' || !value.trim()) {
    return '-'
  }

  const trimmedValue = value.trim()
  const directMatch = trimmedValue.match(/^(\d{4}-\d{2}-\d{2})[ T](\d{2}:\d{2})/)
  if (directMatch) {
    return `${directMatch[1]} ${directMatch[2]}`
  }

  const parsedDate = new Date(trimmedValue)
  if (Number.isNaN(parsedDate.getTime())) {
    return trimmedValue
  }

  const year = String(parsedDate.getFullYear())
  const month = String(parsedDate.getMonth() + 1).padStart(2, '0')
  const day = String(parsedDate.getDate()).padStart(2, '0')
  const hours = String(parsedDate.getHours()).padStart(2, '0')
  const minutes = String(parsedDate.getMinutes()).padStart(2, '0')
  return `${year}-${month}-${day} ${hours}:${minutes}`
}

function formatRemainingDuration(timestamp){
  if (typeof timestamp !== 'number' || Number.isNaN(timestamp)) {
    return '-'
  }

  const remainingSeconds = Math.max(1, Math.floor((timestamp - nowTimestamp.value) / 1000))
  if (remainingSeconds < 60) {
    return `${remainingSeconds}초`
  }

  const remainingMinutes = Math.floor(remainingSeconds / 60)
  if (remainingMinutes < 60) {
    return `${remainingMinutes}분`
  }

  const remainingHours = Math.floor(remainingMinutes / 60)
  if (remainingHours < 24) {
    return `${remainingHours}시간`
  }

  const remainingDays = Math.floor(remainingHours / 24)
  if (remainingDays < 30) {
    return `${remainingDays}일`
  }

  const remainingMonths = Math.floor(remainingDays / 30)
  if (remainingMonths < 12) {
    return `${remainingMonths}달`
  }

  return `${Math.floor(remainingDays / 365)}년`
}

async function loadMySubmissionBan(){
  if (!isAuthenticated.value || !authState.token) {
    submissionBan.value = {
      submission_banned_until: null,
      timestamp: null,
      label: ''
    }
    isLoadingSubmissionBan.value = false
    return null
  }

  isLoadingSubmissionBan.value = true

  try {
    const response = await getMySubmissionBan(authState.token)
    const submissionBannedUntil =
      typeof response?.submission_banned_until === 'string'
        ? response.submission_banned_until
        : null
    const normalizedSubmissionBan = normalizeDateTime(submissionBannedUntil)
    submissionBan.value = {
      submission_banned_until: submissionBannedUntil,
      timestamp: normalizedSubmissionBan.timestamp,
      label: normalizedSubmissionBan.label
    }
    return submissionBan.value
  } catch {
    submissionBan.value = {
      submission_banned_until: null,
      timestamp: null,
      label: ''
    }
    return null
  } finally {
    isLoadingSubmissionBan.value = false
  }
}

function startSubmissionBanRefresh(){
  stopSubmissionBanRefresh()
  nowTimestamp.value = Date.now()
  submissionBanRefreshTimer = window.setInterval(() => {
    nowTimestamp.value = Date.now()
  }, 30_000)
}

function stopSubmissionBanRefresh(){
  if (submissionBanRefreshTimer) {
    clearInterval(submissionBanRefreshTimer)
    submissionBanRefreshTimer = null
  }
}

function scheduleEditorResize(){
  nextTick(() => {
    resizeEditorHeight()
  })
}

function resizeEditorHeight(){
  if (!sourceEditorElement.value) {
    return
  }

  sourceEditorElement.value.style.height = 'auto'
  sourceEditorElement.value.style.height = `${Math.max(sourceEditorElement.value.scrollHeight, 320)}px`
}

function handleSourceEditorInput(){
  scheduleEditorResize()
}

function handleEditorKeydown(event){
  if ((event.ctrlKey || event.metaKey) && event.key === 'Enter') {
    event.preventDefault()
    void submitSolution()
    return
  }

  if (event.key !== 'Tab' || !sourceEditorElement.value || isSubmittingSubmission.value) {
    return
  }

  event.preventDefault()

  const textarea = sourceEditorElement.value
  const selectionStart = textarea.selectionStart
  const selectionEnd = textarea.selectionEnd

  if (event.shiftKey) {
    outdentEditorSelection(selectionStart, selectionEnd)
    return
  }

  indentEditorSelection(selectionStart, selectionEnd)
}

function indentEditorSelection(selectionStart, selectionEnd){
  const indentText = '    '

  if (selectionStart === selectionEnd) {
    const nextValue =
      `${sourceCode.value.slice(0, selectionStart)}${indentText}${sourceCode.value.slice(selectionEnd)}`
    updateEditorValue(
      nextValue,
      selectionStart + indentText.length,
      selectionStart + indentText.length
    )
    return
  }

  const blockRange = getSelectedLineRange(sourceCode.value, selectionStart, selectionEnd)
  const selectedBlock = sourceCode.value.slice(blockRange.lineStart, blockRange.lineEnd)
  const selectedLines = selectedBlock.split('\n')
  const indentedBlock = selectedLines
    .map((line) => `${indentText}${line}`)
    .join('\n')

  const nextValue =
    `${sourceCode.value.slice(0, blockRange.lineStart)}${indentedBlock}${sourceCode.value.slice(blockRange.lineEnd)}`

  updateEditorValue(
    nextValue,
    selectionStart + indentText.length,
    selectionEnd + (indentText.length * selectedLines.length)
  )
}

function outdentEditorSelection(selectionStart, selectionEnd){
  const blockRange = getSelectedLineRange(sourceCode.value, selectionStart, selectionEnd)
  const selectedBlock = sourceCode.value.slice(blockRange.lineStart, blockRange.lineEnd)
  const selectedLines = selectedBlock.split('\n')
  const removedIndentSizes = selectedLines.map((line) => getOutdentWidth(line))
  const totalRemovedIndent = removedIndentSizes.reduce((sum, width) => sum + width, 0)

  if (!totalRemovedIndent) {
    return
  }

  const outdentedBlock = selectedLines
    .map((line, index) => line.slice(removedIndentSizes[index]))
    .join('\n')

  const nextValue =
    `${sourceCode.value.slice(0, blockRange.lineStart)}${outdentedBlock}${sourceCode.value.slice(blockRange.lineEnd)}`

  const removedBeforeSelectionStart =
    selectionStart > blockRange.lineStart ? removedIndentSizes[0] : 0
  const nextSelectionStart =
    selectionStart === selectionEnd
      ? Math.max(blockRange.lineStart, selectionStart - removedIndentSizes[0])
      : Math.max(blockRange.lineStart, selectionStart - removedBeforeSelectionStart)
  const nextSelectionEnd = Math.max(nextSelectionStart, selectionEnd - totalRemovedIndent)

  updateEditorValue(
    nextValue,
    nextSelectionStart,
    nextSelectionEnd
  )
}

function getSelectedLineRange(value, selectionStart, selectionEnd){
  const lineStart = value.lastIndexOf('\n', Math.max(0, selectionStart - 1)) + 1
  const normalizedSelectionEnd =
    selectionEnd > selectionStart && value[selectionEnd - 1] === '\n'
      ? selectionEnd - 1
      : selectionEnd
  const lineEnd = value.indexOf('\n', normalizedSelectionEnd)

  return {
    lineStart,
    lineEnd: lineEnd === -1 ? value.length : lineEnd
  }
}

function getOutdentWidth(line){
  if (line.startsWith('\t')) {
    return 1
  }

  let leadingSpaceCount = 0
  while (leadingSpaceCount < 4 && line[leadingSpaceCount] === ' ') {
    leadingSpaceCount += 1
  }
  return leadingSpaceCount
}

function updateEditorValue(nextValue, selectionStart, selectionEnd){
  sourceCode.value = nextValue

  nextTick(() => {
    if (!sourceEditorElement.value) {
      return
    }

    sourceEditorElement.value.focus()
    sourceEditorElement.value.setSelectionRange(selectionStart, selectionEnd)
    resizeEditorHeight()
  })
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
    if (error?.code === 'submission_banned') {
      const submissionBanStatus = await loadMySubmissionBan()
      if (submissionBanStatus?.submission_banned_until) {
        submitErrorMessage.value =
          `제출이 ${formatTimestamp(submissionBanStatus.submission_banned_until)}까지 제한되어 있습니다.`
      } else {
        submitErrorMessage.value = '현재 제출이 제한되어 있습니다.'
      }
    } else {
      submitErrorMessage.value = formatApiError(error, {
        fallback: '제출을 처리하지 못했습니다.',
        fieldLabels: submissionFieldLabels
      })
    }
  } finally {
    isSubmittingSubmission.value = false
  }
}

onMounted(() => {
  initializeAuth()
  loadProblemDetail()
  loadSupportedLanguageList()
  scheduleEditorResize()
  startSubmissionBanRefresh()
})

onBeforeUnmount(() => {
  stopSubmissionBanRefresh()
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
