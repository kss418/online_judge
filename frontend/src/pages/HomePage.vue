<template>
  <section class="page-grid">
    <article class="panel hero-panel">
      <div class="panel-header">
        <div>
          <p class="panel-kicker">overview</p>
          <h3>프론트 시작점이 준비됐습니다</h3>
        </div>
        <StatusBadge
          :label="statusLabel"
          :tone="statusTone"
        />
      </div>

      <p class="lead-copy">
        이 화면은 백엔드와 연결되는 가장 얇은 기본 셸입니다. 지금은 시스템 상태와 지원 언어만 확인하고,
        다음 단계에서 로그인/문제목록/제출 화면을 차례대로 붙이면 됩니다.
      </p>

      <div class="command-grid">
        <div class="command-card">
          <p class="command-label">backend</p>
          <code>cd backend && HTTP_PORT=8080 ./http_server</code>
        </div>
        <div class="command-card">
          <p class="command-label">frontend</p>
          <code>cd frontend && npm run dev</code>
        </div>
      </div>
    </article>

    <article class="panel">
      <div class="panel-header">
        <div>
          <p class="panel-kicker">health</p>
          <h3>백엔드 연결 확인</h3>
        </div>
      </div>

      <div v-if="isLoading" class="empty-state">
        <p>백엔드 상태를 확인하는 중입니다.</p>
      </div>

      <div v-else-if="errorMessage" class="empty-state error-state">
        <p>{{ errorMessage }}</p>
      </div>

      <div v-else class="info-stack">
        <div class="metric-row">
          <span class="metric-label">message</span>
          <strong>{{ healthMessage }}</strong>
        </div>
        <div class="metric-row">
          <span class="metric-label">api base</span>
          <code>{{ apiBaseUrl }}</code>
        </div>
      </div>
    </article>

    <article class="panel">
      <div class="panel-header">
        <div>
          <p class="panel-kicker">languages</p>
          <h3>지원 언어</h3>
        </div>
      </div>

      <div v-if="isLoading" class="empty-state">
        <p>언어 목록을 불러오는 중입니다.</p>
      </div>

      <div v-else-if="errorMessage" class="empty-state error-state">
        <p>연결이 복구되면 목록이 여기에 표시됩니다.</p>
      </div>

      <div v-else class="language-list">
        <div
          v-for="language in languages"
          :key="language.language"
          class="language-card"
        >
          <strong>{{ language.language }}</strong>
          <span>{{ language.source_extension }}</span>
        </div>
      </div>
    </article>

    <article class="panel">
      <div class="panel-header">
        <div>
          <p class="panel-kicker">next steps</p>
          <h3>다음 작업 추천</h3>
        </div>
      </div>

      <div class="todo-list">
        <div class="todo-item">
          <strong>1. 로그인 페이지 연결</strong>
          <p>회원가입, 로그인, 토큰 갱신 흐름을 먼저 붙이면 이후 페이지 접근 제어가 편해집니다.</p>
        </div>
        <div class="todo-item">
          <strong>2. 문제 목록 화면 구현</strong>
          <p>검색/필터/정렬보다 먼저 기본 목록과 상세 이동부터 붙이는 편이 빠릅니다.</p>
        </div>
        <div class="todo-item">
          <strong>3. 제출 목록과 상세</strong>
          <p>채점 상태와 결과를 보는 화면을 분리하면 운영과 디버깅이 쉬워집니다.</p>
        </div>
      </div>
    </article>
  </section>
</template>

<script setup>
import { computed, onMounted, ref } from 'vue'

import { apiBaseUrl, getSupportedLanguages, getSystemHealth } from '@/api/http'
import StatusBadge from '@/components/StatusBadge.vue'

const isLoading = ref(true)
const errorMessage = ref('')
const healthMessage = ref('unknown')
const languages = ref([])

const statusLabel = computed(() => {
  if (isLoading.value) {
    return 'Loading'
  }

  return errorMessage.value ? 'Offline' : 'Connected'
})

const statusTone = computed(() => {
  if (isLoading.value) {
    return 'neutral'
  }

  return errorMessage.value ? 'danger' : 'success'
})

async function loadOverview(){
  isLoading.value = true
  errorMessage.value = ''

  try {
    const [healthResponse, languageResponse] = await Promise.all([
      getSystemHealth(),
      getSupportedLanguages()
    ])

    healthMessage.value = healthResponse.message || 'ok'
    languages.value = Array.isArray(languageResponse.languages)
      ? languageResponse.languages
      : []
  } catch (error) {
    errorMessage.value = error instanceof Error
      ? error.message
      : '백엔드와 연결할 수 없습니다.'
  } finally {
    isLoading.value = false
  }
}

onMounted(() => {
  loadOverview()
})
</script>
