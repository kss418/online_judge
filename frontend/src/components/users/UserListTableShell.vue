<template>
  <article class="panel user-list-table-shell">
    <div class="user-list-table-shell__toolbar">
      <div>
        <p class="panel-kicker">{{ kicker }}</p>
        <h3>{{ title }}</h3>
        <p v-if="description" class="user-list-table-shell__copy">
          {{ description }}
        </p>
      </div>

      <div class="user-list-table-shell__actions">
        <StatusBadge
          :label="statusLabel"
          :tone="statusTone"
        />

        <div class="user-list-table-shell__search-row">
          <form class="user-list-table-shell__search" @submit.prevent="$emit('submitSearch')">
            <label class="sr-only" :for="searchInputId">유저 검색</label>
            <input
              :id="searchInputId"
              :value="searchInput"
              class="user-list-table-shell__search-input"
              type="search"
              :placeholder="searchPlaceholder"
              @input="$emit('update:searchInput', $event.target.value)"
            />
            <button
              type="submit"
              class="primary-button user-list-table-shell__search-button"
              :disabled="isLoading"
            >
              검색
            </button>
          </form>

          <button
            v-if="appliedQuery"
            type="button"
            class="ghost-button user-list-table-shell__reset-button"
            :disabled="isLoading"
            @click="$emit('resetSearch')"
          >
            검색 초기화
          </button>
        </div>

        <button
          v-if="showRefresh"
          type="button"
          class="ghost-button user-list-table-shell__refresh-button"
          :disabled="isLoading"
          @click="$emit('refresh')"
        >
          새로고침
        </button>
      </div>
    </div>

    <slot name="summary" />

    <div v-if="viewState !== 'ready'" class="empty-state" :class="{ 'error-state': isErrorState }">
      <p>{{ resolvedMessage }}</p>
    </div>

    <div v-else class="user-list-table-shell__table">
      <div class="user-list-table-shell__table-head" :style="headStyle">
        <span
          v-for="column in columns"
          :key="column"
        >
          {{ column }}
        </span>
      </div>

      <div
        v-for="row in rows"
        :key="row.user_id"
        class="user-list-table-shell__row"
        :style="rowStyle"
      >
        <slot name="row" :row="row" />
      </div>
    </div>

    <PaginationBar
      v-if="viewState === 'ready' && totalPages > 1"
      v-model:jump-input="pageJumpInputModel"
      :current-page="currentPage"
      :total-pages="totalPages"
      :is-loading="isLoading"
      :items="paginationItems"
      :jump-input-id="jumpInputId"
      :jump-placeholder="jumpPlaceholder"
      @page-change="$emit('pageChange', $event)"
      @jump-submit="$emit('jumpSubmit')"
    />
  </article>
</template>

<script setup>
import { computed } from 'vue'

import PaginationBar from '@/components/PaginationBar.vue'
import StatusBadge from '@/components/StatusBadge.vue'

const props = defineProps({
  kicker: {
    type: String,
    required: true
  },
  title: {
    type: String,
    required: true
  },
  description: {
    type: String,
    default: ''
  },
  statusLabel: {
    type: String,
    required: true
  },
  statusTone: {
    type: String,
    default: 'success'
  },
  searchInput: {
    type: String,
    default: ''
  },
  appliedQuery: {
    type: String,
    default: ''
  },
  searchInputId: {
    type: String,
    required: true
  },
  searchPlaceholder: {
    type: String,
    default: 'ID 검색'
  },
  showRefresh: {
    type: Boolean,
    default: false
  },
  isLoading: {
    type: Boolean,
    default: false
  },
  viewState: {
    type: String,
    default: 'ready'
  },
  viewMessage: {
    type: String,
    default: ''
  },
  emptyMessage: {
    type: String,
    default: '표시할 사용자가 아직 없습니다.'
  },
  columns: {
    type: Array,
    required: true
  },
  rows: {
    type: Array,
    required: true
  },
  gridTemplateColumns: {
    type: String,
    required: true
  },
  tableMinWidth: {
    type: String,
    default: ''
  },
  rowIntrinsicSize: {
    type: String,
    default: '74px'
  },
  currentPage: {
    type: Number,
    required: true
  },
  totalPages: {
    type: Number,
    required: true
  },
  pageJumpInput: {
    type: String,
    default: ''
  },
  paginationItems: {
    type: Array,
    required: true
  },
  jumpInputId: {
    type: String,
    required: true
  },
  jumpPlaceholder: {
    type: String,
    required: true
  }
})

const emit = defineEmits([
  'update:searchInput',
  'submitSearch',
  'resetSearch',
  'refresh',
  'update:pageJumpInput',
  'pageChange',
  'jumpSubmit'
])

const pageJumpInputModel = computed({
  get(){
    return props.pageJumpInput
  },
  set(value){
    emit('update:pageJumpInput', value)
  }
})

const isErrorState = computed(() =>
  props.viewState === 'error' || props.viewState === 'denied'
)
const resolvedMessage = computed(() =>
  props.viewState === 'empty' ? props.emptyMessage : props.viewMessage
)
const headStyle = computed(() => ({
  gridTemplateColumns: props.gridTemplateColumns,
  minWidth: props.tableMinWidth || undefined
}))
const rowStyle = computed(() => ({
  gridTemplateColumns: props.gridTemplateColumns,
  minWidth: props.tableMinWidth || undefined,
  containIntrinsicSize: props.rowIntrinsicSize
}))
</script>

<style scoped>
.user-list-table-shell {
  display: grid;
  gap: 1rem;
}

.user-list-table-shell__toolbar {
  display: flex;
  justify-content: space-between;
  gap: 1rem;
  align-items: start;
}

.user-list-table-shell__copy {
  margin: 0.45rem 0 0;
  color: var(--ink-soft);
  white-space: pre-line;
}

.user-list-table-shell__actions {
  display: flex;
  flex-wrap: wrap;
  gap: 0.75rem;
  align-items: center;
  justify-content: flex-end;
  margin-left: auto;
  min-width: 0;
}

.user-list-table-shell__search-row {
  display: flex;
  justify-content: flex-end;
  gap: 1rem;
  align-items: center;
  flex-wrap: wrap;
  min-width: 0;
}

.user-list-table-shell__search {
  display: flex;
  justify-content: flex-end;
  gap: 0.75rem;
  align-items: center;
  min-width: 0;
}

.user-list-table-shell__search-input {
  width: min(100%, 300px);
  min-height: 2.9rem;
  padding: 0.8rem 0.95rem;
  border-radius: 999px;
  border: 1px solid rgba(20, 33, 61, 0.14);
  background: rgba(255, 255, 255, 0.92);
  color: var(--ink-strong);
  font: inherit;
}

.user-list-table-shell__search-input:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}

.user-list-table-shell__search-button,
.user-list-table-shell__reset-button,
.user-list-table-shell__refresh-button {
  flex-shrink: 0;
}

.user-list-table-shell__table {
  border: 1px solid var(--line);
  border-radius: 22px;
  overflow: hidden;
  background: rgba(255, 255, 255, 0.68);
}

.user-list-table-shell__table-head,
.user-list-table-shell__row {
  display: grid;
  gap: 1rem;
  align-items: center;
  padding: 1rem 1.15rem;
}

.user-list-table-shell__table-head {
  font-size: 0.88rem;
  font-weight: 700;
  color: var(--ink-soft);
  background: rgba(20, 33, 61, 0.04);
  border-bottom: 1px solid var(--line);
}

.user-list-table-shell__row {
  content-visibility: auto;
}

.user-list-table-shell__row + .user-list-table-shell__row {
  border-top: 1px solid var(--line);
}

.sr-only {
  position: absolute;
  width: 1px;
  height: 1px;
  padding: 0;
  margin: -1px;
  overflow: hidden;
  clip: rect(0, 0, 0, 0);
  white-space: nowrap;
  border: 0;
}

@media (max-width: 1120px) {
  .user-list-table-shell__toolbar {
    flex-direction: column;
  }

  .user-list-table-shell__actions {
    width: 100%;
    justify-content: flex-start;
    margin-left: 0;
  }

  .user-list-table-shell__search-row,
  .user-list-table-shell__search {
    justify-content: flex-start;
  }

  .user-list-table-shell__table {
    overflow-x: auto;
  }
}

@media (max-width: 720px) {
  .user-list-table-shell__actions,
  .user-list-table-shell__search-row,
  .user-list-table-shell__search {
    flex-direction: column;
    align-items: stretch;
  }

  .user-list-table-shell__search-input {
    width: 100%;
    min-width: 0;
  }
}
</style>
