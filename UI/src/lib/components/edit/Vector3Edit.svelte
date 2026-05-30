<script lang="ts">
  // Define the vector object state
  let {vector = $bindable()}: {vector: Vector3} = $props();
</script>

<div class="vec3">
	<div class="axis ax-x">
		<span class="ax-label">X</span>
		<input type="number" bind:value={vector.x} step="0.1" />
	</div>
	<div class="axis ax-y">
		<span class="ax-label">Y</span>
		<input type="number" bind:value={vector.y} step="0.1" />
	</div>
	<div class="axis ax-z">
		<span class="ax-label">Z</span>
		<input type="number" bind:value={vector.z} step="0.1" />
	</div>
</div>
 
<style>
	@import url('https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500&display=swap');
 
	.vec3 {
		display: flex;
		gap: 2px;
		width: 100%;
	}
 
	.axis {
		flex: 1;
		display: flex;
		align-items: stretch;
		overflow: hidden;
		border: 1px solid var(--border);
		background: var(--bg-base);
		transition: border-color 0.15s;
		min-width: 0;
	}
 
	.axis:focus-within { border-color: var(--accent); }
 
	.ax-label {
		display: flex;
		align-items: center;
		justify-content: center;
		min-width: 20px;
		font-family: 'IBM Plex Mono', monospace;
		font-size: 10px;
		font-weight: 500;
		color: #fff;
		padding: 0 5px;
		flex-shrink: 0;
		letter-spacing: 0;
	}
 
	.ax-x .ax-label { background: var(--ax); }
	.ax-y .ax-label { background: var(--ay); }
	.ax-z .ax-label { background: var(--az); }
 
	input {
		flex: 1;
		background: transparent;
		border: none;
		color: var(--text);
		padding: 4px 5px;
		font-family: 'IBM Plex Mono', monospace;
		font-size: 11px;
		outline: none;
		width: 0;
		min-width: 0;
		text-align: right;
	}
 
	input::-webkit-inner-spin-button,
	input::-webkit-outer-spin-button {
		opacity: 0.25;
	}
</style>
