<template>
  <section class="page-grid">
    <article class="panel hero-panel">
      <div class="panel-header">
        <div>
          <p class="panel-kicker">overview</p>
          <h3>문제를 풀고 제출 결과를 확인해보세요</h3>
        </div>
        <StatusBadge
          :label="statusLabel"
          :tone="statusTone"
        />
      </div>

      <p class="lead-copy">
        이 사이트에서는 문제를 탐색하고 코드를 제출한 뒤 채점 결과와 제출 기록을 한곳에서 확인할 수
        있습니다. 로그인하면 내가 이미 해결한 문제인지, 아직 시도 중인 문제인지도 함께 볼 수 있습니다.
      </p>

      <div class="command-grid">
        <div class="command-card">
          <p class="command-label">browse</p>
          <strong class="overview-feature-title">문제 탐색</strong>
          <p class="overview-feature-copy">
            문제 목록에서 번호와 제목을 빠르게 찾고, 상세 페이지에서 제한과 예제를 확인할 수 있습니다.
          </p>
        </div>
        <div class="command-card">
          <p class="command-label">submit</p>
          <strong class="overview-feature-title">풀이 제출</strong>
          <p class="overview-feature-copy">
            지원 언어로 코드를 제출하고 채점 상태가 바뀌는 흐름을 바로 확인할 수 있습니다.
          </p>
        </div>
        <div class="command-card">
          <p class="command-label">track</p>
          <strong class="overview-feature-title">기록 확인</strong>
          <p class="overview-feature-copy">
            제출 목록과 문제 상태 색상으로 내 풀이 진행 상황과 최근 결과를 한눈에 볼 수 있습니다.
          </p>
        </div>
      </div>
    </article>

    <article class="panel">
      <div class="panel-header">
        <div>
          <p class="panel-kicker">health</p>
          <h3>서비스 상태</h3>
        </div>
      </div>

      <div v-if="isLoading" class="empty-state">
        <p>서비스 상태를 확인하는 중입니다.</p>
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
  </section>
</template>

<script setup>
import StatusBadge from '@/components/StatusBadge.vue'
import { useHomePage } from '@/composables/useHomePage'

const {
  apiBaseUrl,
  isLoading,
  errorMessage,
  healthMessage,
  languages,
  statusLabel,
  statusTone
} = useHomePage()
</script>
