<template>
  <Teleport to="body">
    <div
      v-if="open"
      class="submission-history-backdrop"
      @click.self="$emit('close')"
    >
      <section
        class="submission-history-dialog"
        role="dialog"
        aria-modal="true"
        aria-labelledby="submission-history-title"
      >
        <div class="submission-history-header">
          <div>
            <p class="panel-kicker">submission history</p>
            <h3 id="submission-history-title">
              제출 #{{ activeSubmissionId ? formatCount(activeSubmissionId) : '' }} 채점 내역
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
          <p>채점 내역을 불러오는 중입니다.</p>
        </div>

        <div v-else-if="errorMessage" class="empty-state error-state">
          <p>{{ errorMessage }}</p>
        </div>

        <template v-else>
          <p
            v-if="shouldPoll"
            class="submission-history-live"
          >
            자동 갱신 중
          </p>

          <div
            v-if="entries.length"
            class="submission-history-timeline"
          >
            <article
              v-for="(historyEntry, historyIndex) in entries"
              :key="historyEntry.history_key"
              class="submission-history-entry"
              :class="{ 'is-current': historyIndex === entries.length - 1 }"
            >
              <span class="submission-history-entry-marker" aria-hidden="true"></span>
              <div class="submission-history-entry-card">
                <div class="submission-history-entry-top">
                  <SubmissionStatusBadge :status="historyEntry.to_status" />
                  <span class="submission-history-entry-time">
                    {{ historyEntry.created_at_label }}
                  </span>
                </div>
                <p class="submission-history-entry-transition">
                  {{ formatHistoryTransition(historyEntry) }}
                </p>
                <p
                  v-if="historyEntry.reason"
                  class="submission-history-entry-reason"
                >
                  사유: {{ historyEntry.reason }}
                </p>
              </div>
            </article>
          </div>

          <div v-else class="empty-state submission-history-empty-state">
            <p>히스토리가 아직 없습니다.</p>
          </div>
        </template>
      </section>
    </div>
  </Teleport>
</template>

<script setup>
import SubmissionStatusBadge from '@/components/submissions/SubmissionStatusBadge.vue'

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
  entries: {
    type: Array,
    required: true
  },
  activeSubmissionId: {
    type: Number,
    default: null
  },
  shouldPoll: {
    type: Boolean,
    required: true
  },
  formatCount: {
    type: Function,
    required: true
  },
  formatHistoryTransition: {
    type: Function,
    required: true
  }
})

defineEmits(['close'])
</script>

<style scoped>
.submission-history-backdrop {
  position: fixed;
  inset: 0;
  z-index: 41;
  display: grid;
  place-items: center;
  padding: 1.5rem;
  background: rgba(15, 23, 42, 0.42);
  backdrop-filter: blur(10px);
}

.submission-history-dialog {
  width: min(760px, 100%);
  max-height: calc(100vh - 3rem);
  overflow: auto;
  border: 1px solid rgba(255, 255, 255, 0.2);
  border-radius: 24px;
  background: rgba(255, 255, 255, 0.95);
  box-shadow: var(--shadow);
  padding: 1.4rem;
  display: grid;
  gap: 1rem;
}

.submission-history-header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 1rem;
}

.submission-history-live {
  margin: 0;
  color: var(--ink-soft);
  font-size: 0.88rem;
  font-weight: 700;
}

.submission-history-timeline {
  position: relative;
  display: grid;
  gap: 0.95rem;
  padding-left: 0.25rem;
}

.submission-history-timeline::before {
  content: '';
  position: absolute;
  left: 0.7rem;
  top: 0.5rem;
  bottom: 0.5rem;
  width: 1px;
  background: rgba(20, 33, 61, 0.12);
}

.submission-history-entry {
  position: relative;
  display: grid;
  grid-template-columns: auto minmax(0, 1fr);
  gap: 0.9rem;
  align-items: start;
}

.submission-history-entry-marker {
  position: relative;
  z-index: 1;
  width: 1.4rem;
  height: 1.4rem;
  margin-top: 0.7rem;
  border-radius: 999px;
  border: 4px solid rgba(217, 119, 6, 0.18);
  background: white;
  box-shadow: 0 0 0 4px rgba(255, 255, 255, 0.95);
}

.submission-history-entry.is-current .submission-history-entry-marker {
  border-color: rgba(217, 119, 6, 0.32);
}

.submission-history-entry-card {
  display: grid;
  gap: 0.65rem;
  padding: 1rem 1.05rem;
  border: 1px solid rgba(20, 33, 61, 0.08);
  border-radius: 20px;
  background: rgba(255, 255, 255, 0.88);
}

.submission-history-entry.is-current .submission-history-entry-card {
  border-color: rgba(217, 119, 6, 0.22);
  background: rgba(255, 247, 237, 0.9);
}

.submission-history-entry-top {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 0.75rem;
  flex-wrap: wrap;
}

.submission-history-entry-time {
  color: var(--ink-soft);
  font-size: 0.88rem;
  font-weight: 700;
  white-space: nowrap;
}

.submission-history-entry-transition {
  margin: 0;
  color: var(--ink-soft);
  font-size: 0.9rem;
  font-weight: 700;
}

.submission-history-entry-reason {
  margin: 0;
  padding: 0.8rem 0.9rem;
  border-radius: 16px;
  background: rgba(20, 33, 61, 0.05);
  color: var(--ink-strong);
  font-size: 0.92rem;
  line-height: 1.6;
  white-space: pre-wrap;
  word-break: break-word;
}

.submission-history-empty-state {
  margin-top: 0.25rem;
}

@media (max-width: 720px) {
  .submission-history-dialog {
    width: 100%;
  }

  .submission-history-entry-top {
    align-items: flex-start;
  }
}
</style>
