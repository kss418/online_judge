<template>
  <div class="pagination-bar">
    <div class="pagination-controls">
      <button
        type="button"
        class="ghost-button pagination-button"
        :disabled="currentPage === 1"
        @click="$emit('pageChange', currentPage - 1)"
      >
        이전
      </button>

      <div class="pagination-pages">
        <template
          v-for="item in items"
          :key="item.key"
        >
          <button
            v-if="item.type === 'page'"
            type="button"
            class="pagination-page"
            :class="{ 'is-active': item.value === currentPage }"
            @click="$emit('pageChange', item.value)"
          >
            {{ item.value }}
          </button>
          <span
            v-else
            class="pagination-ellipsis"
            aria-hidden="true"
          >
            ...
          </span>
        </template>
      </div>

      <button
        type="button"
        class="ghost-button pagination-button"
        :disabled="currentPage === totalPages"
        @click="$emit('pageChange', currentPage + 1)"
      >
        다음
      </button>
    </div>

    <form class="pagination-jump" @submit.prevent="$emit('jumpSubmit')">
      <label class="sr-only" :for="jumpInputId">페이지 이동</label>
      <input
        :id="jumpInputId"
        :value="jumpInput"
        class="pagination-jump-input"
        type="number"
        inputmode="numeric"
        min="1"
        :max="totalPages"
        :placeholder="jumpPlaceholder"
        @input="$emit('update:jumpInput', $event.target.value)"
      />
      <button
        type="submit"
        class="ghost-button pagination-jump-button"
        :disabled="isLoading"
      >
        이동
      </button>
    </form>
  </div>
</template>

<script setup>
defineProps({
  currentPage: {
    type: Number,
    required: true
  },
  totalPages: {
    type: Number,
    required: true
  },
  isLoading: {
    type: Boolean,
    default: false
  },
  items: {
    type: Array,
    required: true
  },
  jumpInput: {
    type: String,
    default: ''
  },
  jumpInputId: {
    type: String,
    required: true
  },
  jumpPlaceholder: {
    type: String,
    default: ''
  }
})

defineEmits([
  'pageChange',
  'jumpSubmit',
  'update:jumpInput'
])
</script>

<style scoped>
.pagination-bar {
  display: flex;
  justify-content: space-between;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
  padding-top: 0.25rem;
}

.pagination-controls {
  display: flex;
  justify-content: center;
  gap: 0.75rem;
  align-items: center;
  flex-wrap: wrap;
}

.pagination-pages {
  display: flex;
  gap: 0.55rem;
  flex-wrap: wrap;
  justify-content: center;
}

.pagination-button {
  min-width: 88px;
}

.pagination-jump {
  display: flex;
  gap: 0.6rem;
  align-items: center;
  margin-left: auto;
}

.pagination-jump-input {
  width: 7rem;
  min-height: 2.75rem;
  padding: 0.65rem 0.9rem;
  border-radius: 999px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.92);
  color: var(--ink-strong);
  font: inherit;
}

.pagination-jump-input:focus {
  outline: 2px solid rgba(217, 119, 6, 0.18);
  border-color: rgba(217, 119, 6, 0.5);
}

.pagination-page {
  min-width: 2.75rem;
  min-height: 2.75rem;
  padding: 0.65rem 0.9rem;
  border-radius: 999px;
  border: 1px solid var(--line);
  background: rgba(255, 255, 255, 0.72);
  color: var(--ink-strong);
  font: inherit;
  font-weight: 700;
  cursor: pointer;
  transition:
    transform 160ms ease,
    background 160ms ease,
    border-color 160ms ease;
}

.pagination-page:hover {
  transform: translateY(-1px);
}

.pagination-page.is-active {
  color: white;
  background: linear-gradient(135deg, #d97706, #ea580c);
  border-color: transparent;
}

.pagination-ellipsis {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 2.75rem;
  min-height: 2.75rem;
  color: var(--ink-soft);
  font-weight: 700;
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

@media (max-width: 900px) {
  .pagination-bar {
    flex-direction: column;
    align-items: stretch;
  }

  .pagination-controls,
  .pagination-jump {
    justify-content: center;
    margin-left: 0;
  }

  .pagination-jump-input {
    width: 100%;
  }
}
</style>
