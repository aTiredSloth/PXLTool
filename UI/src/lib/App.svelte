<script lang="ts">
	import { onMount } from "svelte";
   import SceneComponent from "./components/SceneComponent.svelte";
	
	let Scenes = $state<Scene[]>([]);
	let NewSceneName = $state("NewScene");
	let CurrentScene = $state<Scene | null>();
	
	async function LoadScenes()
	{
		const response = await fetch('/api/scenes');
		let LoadedScenesJson = await response.json();
		let LoadedScenes: Scene[] = [];
		for (const item of LoadedScenesJson)
		{
			console.log(item);
			LoadedScenes.push(item);
		}

		Scenes = LoadedScenes;
	}

	onMount(()=>{LoadScenes()})

	async function CreateScene()
	{
		await fetch('/api/newscene/' + NewSceneName);
		LoadScenes();
	}
</script>


<div class="app">
	<header class="titlebar">
		<div class="titlebar-logo">
			<span class="logo-mark">◈</span>
			<span class="logo-name">PXLTool</span>
		</div>
		<div class="titlebar-subtitle">3D Model → Sprite Sheet Compiler</div>
		<div class="titlebar-right">
			<span class="version-tag">v0.1.0</span>
		</div>
	</header>
 
	<div class="workspace">
		<aside class="sidebar">
			<div class="panel-header">
				<span class="ph-icon">⬡</span>
				SCENES
			</div>
 
			<div class="new-scene-row">
				<input
					class="engine-input"
					bind:value={NewSceneName}
					placeholder="Scene name..."
					spellcheck="false"
				/>
				<button class="btn-create" onclick={CreateScene}>
					<span class="btn-plus">+</span> NEW
				</button>
			</div>
 
			<div class="scene-list">
				{#each Scenes as SceneId}
					<button
						class="scene-item"
						class:active={CurrentScene === SceneId}
						onclick={() => { CurrentScene = SceneId }}
					>
						<span class="si-arrow">▷</span>
						<span class="si-name">{SceneId.Name}</span>
					</button>
				{/each}
				{#if Scenes.length === 0}
					<div class="list-empty">No scenes — create one above</div>
				{/if}
			</div>
		</aside>
 
		<main class="main-content">
			{#if CurrentScene}
				<SceneComponent bind:Id={CurrentScene} />
			{:else}
				<div class="splash">
					<div class="splash-icon">◈</div>
					<div class="splash-headline">Select or create a scene</div>
					<div class="splash-sub">Configure models, camera, and render a sprite sheet</div>
				</div>
			{/if}
		</main>
	</div>
</div>
 
<style>
	@import url('https://fonts.googleapis.com/css2?family=Chakra+Petch:wght@300;400;500;600;700&family=IBM+Plex+Mono:wght@400;500&display=swap');
 
	:global(*, *::before, *::after) {
		box-sizing: border-box;
		margin: 0;
		padding: 0;
	}
 
	:global(body) {
		background: #0c0c0c;
		color: #c8c8c8;
		font-family: 'Chakra Petch', sans-serif;
		font-size: 12px;
		height: 100vh;
		overflow: hidden;
	}
 
	:global(:root) {
		--bg-base:      #0c0c0c;
		--bg-panel:     #141414;
		--bg-raised:    #1c1c1c;
		--bg-hover:     #222222;
		--bg-active:    #282828;
		--border:       #282828;
		--border-hi:    #383838;
		--text:         #d0d0d0;
		--text-muted:   #6a6a6a;
		--text-dim:     #3e3e3e;
		--accent:       #e07b39;
		--accent-dim:   rgba(224, 123, 57, 0.12);
		--accent-glow:  rgba(224, 123, 57, 0.35);
		--ax:           #c0392b;
		--ay:           #27ae60;
		--az:           #2980b9;
		--ok:           #27ae60;
	}
 
	/* ── App shell ── */
	.app {
		display: flex;
		flex-direction: column;
		height: 100vh;
	}
 
	/* ── Titlebar ── */
	.titlebar {
		height: 38px;
		background: #0a0a0a;
		border-bottom: 1px solid var(--border);
		display: flex;
		align-items: center;
		padding: 0 14px;
		gap: 18px;
		flex-shrink: 0;
		user-select: none;
	}
 
	.titlebar-logo {
		display: flex;
		align-items: center;
		gap: 7px;
		font-size: 13px;
		font-weight: 700;
		letter-spacing: 0.14em;
	}
 
	.logo-mark { color: var(--accent); font-size: 17px; }
	.accent    { color: var(--accent); }
 
	.titlebar-subtitle {
		color: var(--text-muted);
		font-size: 10px;
		letter-spacing: 0.06em;
	}
 
	.titlebar-right { margin-left: auto; }
 
	.version-tag {
		background: var(--bg-raised);
		border: 1px solid var(--border-hi);
		color: var(--text-muted);
		font-size: 9px;
		padding: 2px 7px;
		letter-spacing: 0.12em;
		font-family: 'IBM Plex Mono', monospace;
	}
 
	/* ── Workspace ── */
	.workspace {
		display: flex;
		flex: 1;
		overflow: hidden;
	}
 
	/* ── Sidebar ── */
	.sidebar {
		width: 224px;
		background: var(--bg-panel);
		border-right: 1px solid var(--border);
		display: flex;
		flex-direction: column;
		flex-shrink: 0;
	}
 
	.panel-header {
		height: 30px;
		background: #0e0e0e;
		border-bottom: 1px solid var(--border);
		display: flex;
		align-items: center;
		gap: 7px;
		padding: 0 10px;
		font-size: 10px;
		font-weight: 600;
		letter-spacing: 0.16em;
		color: var(--text-muted);
		flex-shrink: 0;
	}
 
	.ph-icon { color: var(--accent); }
 
	.new-scene-row {
		padding: 7px;
		display: flex;
		gap: 4px;
		border-bottom: 1px solid var(--border);
		flex-shrink: 0;
	}
 
	.engine-input {
		flex: 1;
		background: var(--bg-base);
		border: 1px solid var(--border-hi);
		color: var(--text);
		padding: 5px 8px;
		font-family: 'Chakra Petch', sans-serif;
		font-size: 11px;
		outline: none;
		transition: border-color 0.15s, box-shadow 0.15s;
	}
 
	.engine-input:focus {
		border-color: var(--accent);
		box-shadow: 0 0 0 1px var(--accent-dim);
	}
 
	.engine-input::placeholder { color: var(--text-dim); }
 
	.btn-create {
		background: var(--accent);
		border: none;
		color: #fff;
		padding: 5px 10px;
		font-family: 'Chakra Petch', sans-serif;
		font-size: 10px;
		font-weight: 600;
		letter-spacing: 0.1em;
		cursor: pointer;
		display: flex;
		align-items: center;
		gap: 4px;
		white-space: nowrap;
		transition: background 0.15s, box-shadow 0.15s;
	}
 
	.btn-create:hover {
		background: #f08848;
		box-shadow: 0 0 10px var(--accent-glow);
	}
 
	.btn-plus { font-size: 14px; line-height: 1; }
 
	.scene-list {
		flex: 1;
		overflow-y: auto;
		padding: 4px;
	}
 
	.scene-list::-webkit-scrollbar { width: 3px; }
	.scene-list::-webkit-scrollbar-thumb { background: var(--border-hi); }
 
	.scene-item {
		width: 100%;
		background: transparent;
		border: 1px solid transparent;
		color: var(--text);
		padding: 6px 8px;
		font-family: 'Chakra Petch', sans-serif;
		font-size: 11px;
		cursor: pointer;
		text-align: left;
		display: flex;
		align-items: center;
		gap: 8px;
		transition: background 0.1s, border-color 0.1s;
		margin-bottom: 1px;
	}
 
	.scene-item:hover       { background: var(--bg-hover); border-color: var(--border-hi); }
	.scene-item.active      { background: var(--accent-dim); border-color: var(--accent); }
	.scene-item.active .si-arrow { color: var(--accent); }
 
	.si-arrow { color: var(--text-dim); font-size: 9px; flex-shrink: 0; }
	.si-name  { overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
 
	.list-empty {
		color: var(--text-dim);
		font-size: 10px;
		padding: 18px 8px;
		text-align: center;
		line-height: 1.6;
	}
 
	/* ── Main ── */
	.main-content {
		flex: 1;
		overflow: hidden;
		background: var(--bg-base);
	}
 
	.splash {
		display: flex;
		flex-direction: column;
		align-items: center;
		justify-content: center;
		height: 100%;
		gap: 10px;
	}
 
	.splash-icon     { font-size: 52px; color: var(--text-dim); opacity: 0.35; }
	.splash-headline { color: var(--text-muted); font-size: 13px; letter-spacing: 0.1em; }
	.splash-sub      { color: var(--text-dim); font-size: 10px; letter-spacing: 0.07em; }
</style>
