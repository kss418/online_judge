<template>
  <Teleport to="body">
    <div
      v-if="open"
      class="submission-source-backdrop"
      @click.self="$emit('close')"
    >
      <section
        class="submission-source-dialog"
        role="dialog"
        aria-modal="true"
        aria-labelledby="submission-source-title"
      >
        <div class="submission-source-header">
          <div>
            <p class="panel-kicker">submission source</p>
            <h3 id="submission-source-title">
              제출 #{{ activeSubmissionId ? formatCount(activeSubmissionId) : '' }} 소스 코드
            </h3>
          </div>
          <button
            type="button"
            class="icon-button"
            aria-label="닫기"
            @click="$emit('close')"
          >
            ×
          </button>
        </div>

        <div v-if="isLoading" class="empty-state">
          <p>소스 코드를 불러오는 중입니다.</p>
        </div>

        <div v-else-if="errorMessage" class="empty-state error-state">
          <p>{{ errorMessage }}</p>
        </div>

        <div v-else-if="sourceDetail" class="submission-source-content">
          <div class="submission-source-meta">
            <StatusBadge :label="sourceDetail.language" tone="neutral" />
          </div>
          <pre class="submission-source-code"><code>{{ sourceDetail.source_code }}</code></pre>
          <div
            v-if="sourceDetail.compile_output || sourceDetail.judge_output"
            class="submission-source-diagnostics"
          >
            <div
              v-if="sourceDetail.compile_output"
              class="submission-source-diagnostic"
            >
              <p class="submission-source-diagnostic-title">컴파일 출력</p>
              <pre class="submission-source-diagnostic-log"><code>{{ sourceDetail.compile_output }}</code></pre>
            </div>
            <div
              v-if="sourceDetail.judge_output"
              class="submission-source-diagnostic"
            >
              <p class="submission-source-diagnostic-title">채점 출력</p>
              <pre class="submission-source-diagnostic-log"><code>{{ sourceDetail.judge_output }}</code></pre>
            </div>
          </div>
          <div class="submission-source-actions">
            <button
              type="button"
              class="ghost-button"
              @click="$emit('copy')"
            >
              {{ copyButtonLabel }}
            </button>
          </div>
        </div>
      </section>
    </div>
  </Teleport>
</template>

<script setup>
import StatusBadge from '@/components/StatusBadge.vue'

defineProps({
  open: {
    type: Boolean,
    required: true
  },
  isLoading: {
    type: Boolean,
    required: true
  },
  errorMessage: {
    type: String,
    required: true
  },
  sourceDetail: {
    type: Object,
    default: null
  },
  activeSubmissionId: {
    type: Number,
    default: null
  },
  copyButtonLabel: {
    type: String,
    required: true
  },
  formatCount: {
    type: Function,
    required: true
  }
})

defineEmits(['close', 'copy'])
</script>

<style scoped>
.submission-source-backdrop {
  position: fixed;
  inset: 0;
  z-index: 40;
  display: grid;
  place-items: center;
  padding: 1.5rem;
  background: rgba(15, 23, 42, 0.42);
  backdrop-filter: blur(10px);
}

.submission-source-dialog {
  width: min(960px, 100%);
  max-height: calc(100vh - 3rem);
  overflow: auto;
  border: 1px solid rgba(255, 255, 255, 0.2);
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.95);
  box-shadow: var(--shadow);
  padding: 1.4rem;
}

.submission-source-header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 1rem;
}

.submission-source-content {
  display: grid;
  gap: 1rem;
}

.submission-source-meta {
  display: flex;
  justify-content: flex-start;
}

.submission-source-diagnostics {
  display: grid;
  gap: 0.9rem;
}

.submission-source-diagnostic {
  display: grid;
  gap: 0.5rem;
}

.submission-source-diagnostic-title {
  margin: 0;
  color: var(--ink-soft);
  font-size: 0.9rem;
  font-weight: 700;
}

.submission-source-diagnostic-log {
  margin: 0;
  padding: 1rem 1.1rem;
  border-radius: 18px;
  border: 1px solid rgba(185, 28, 28, 0.14);
  background: rgba(185, 28, 28, 0.06);
  overflow: auto;
  color: var(--danger);
  font-family: "SFMono-Regular", "Consolas", monospace;
  font-size: 0.9rem;
  line-height: 1.6;
  white-space: pre-wrap;
  word-break: break-word;
}

.submission-source-actions {
  display: flex;
  justify-content: flex-end;
}

.submission-source-code {
  margin: 0;
  padding: 1rem 1.1rem;
  border-radius: 18px;
  background: rgba(20, 33, 61, 0.06);
  overflow: auto;
  color: var(--ink-strong);
  font-family: "SFMono-Regular", "Consolas", monospace;
  font-size: 0.92rem;
  line-height: 1.6;
  white-space: pre-wrap;
  word-break: break-word;
}

@media (max-width: 720px) {
  .submission-source-dialog {
    width: 100%;
  }
}
</style>
