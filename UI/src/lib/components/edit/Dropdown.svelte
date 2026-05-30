<script lang="ts">

	let {CurrentFilter = $bindable(), Options} = $props();
	let Filter: number[] = $state([]);
	async function Refresh()
	{
		Filter = await Options();
	}
</script>

<div class="dropdown">
	<select onclick ={Refresh} bind:value={CurrentFilter} class="engine-select">
		{#each Filter as filter, i}
			<option value={filter}>{filter}</option>
		{/each}
	</select>
	<span class="arrow">▾</span>
</div>
 
<style>
	.dropdown {
		position: relative;
		flex: 1;
		min-width: 0;
	}
 
	.engine-select {
		appearance: none;
		-webkit-appearance: none;
		width: 100%;
		background: var(--bg-base);
		border: 1px solid var(--border);
		color: var(--text);
		padding: 4px 22px 4px 8px;
		font-family: 'Chakra Petch', sans-serif;
		font-size: 11px;
		cursor: pointer;
		outline: none;
		transition: border-color 0.15s, background 0.15s;
		text-overflow: ellipsis;
		overflow: hidden;
		white-space: nowrap;
	}
 
	.engine-select:hover  { border-color: var(--border-hi); }
	.engine-select:focus  { border-color: var(--accent); background: var(--accent-dim); }
 
	.engine-select option {
		background: #1a1a1a;
		color: var(--text);
	}
 
	.arrow {
		position: absolute;
		right: 7px;
		top: 50%;
		transform: translateY(-50%);
		color: var(--text-muted);
		font-size: 10px;
		pointer-events: none;
		line-height: 1;
	}
</style>
