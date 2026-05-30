<script lang="ts">
    import { render } from "svelte/server";
    import Dropdown from "./edit/Dropdown.svelte";
    import Vector3Edit from "./edit/Vector3Edit.svelte";
    import ScenePreview from "./ScenePreview.svelte";
    let files: any = $state();
    let {Id = $bindable()}: {Id:Scene} = $props();
    let View: any = $state();
    async function GetMeshes()
    {
    	let response = await fetch("api/meshes");
     	return await response.json();
    }

    async function GetAnimations()
    {
    	let response = await fetch("api/animations");

     	return await response.json();
    }

    async function GetTextures()
    {
    	let response = await fetch("api/textures");

     	return await response.json();
    }

    async function GetShaders()
    {
    	let response = await fetch("api/shaders");

     	return await response.json();
    }

    async function AddMesh()
    {
    	Id.Models = [...(Id.Models ?? []),
     {
     	MeshId: 0, 
     	AnimationId: 0,
      TextureId: 0,
     	ShaderId: 0, 
     	Location: {x: 0, y: 0, z: 0}, 
     	Rotation: {x: 0, y: 0, z: 0}, 
     	Scale: {x: 1, y: 1, z: 1}
     }
     ];
    }

    async function Render()
    {
   	await fetch("api/scene/" + Id.Name + "/update", {method: "POST", body: JSON.stringify(Id)});
    	
    	View.reload();
    }

    async function Import()
    {
    	for (const file of files)
     	{
    		const formData = new FormData();
      	formData.append("file", file);
      	
      	await fetch("/api/load", {
      	method: "POST",
      	body: formData
      	});
     	}

      return;
    }
</script>
<div class="scene-editor">
 
	<!-- ── Left: Properties ── -->
	<div class="props">
 
		<!-- Import -->
		<div class="section">
			<div class="section-hdr">
				<span class="sh-icon">↑</span> IMPORT ASSETS
			</div>
			<div class="section-body">
				<div class="file-zone">
					<input bind:files id="fu" name="fu" type="file" class="file-hidden" />
					<label for="fu" class="file-label">
						<span class="fl-icon">⬆</span>
						<span>Choose File to Import</span>
					</label>
				</div>
				<button class="btn btn-ghost" onclick={Import}>
					<span>↑</span> IMPORT
				</button>
			</div>
		</div>
 
		<!-- Camera -->
		<div class="section">
			<div class="section-hdr">
				<span class="sh-icon">◎</span> CAMERA
			</div>
			<div class="section-body">
				<div class="prop-group">
					<div class="prop-label">Location</div>
					<Vector3Edit bind:vector={Id.Camera.Location} />
				</div>
				<div class="prop-group">
					<div class="prop-label">Rotation</div>
					<Vector3Edit bind:vector={Id.Camera.Rotation} />
				</div>
 
				<div class="divider"></div>
 
				<div class="prop-row">
					<span class="prop-key">Projection</span>
					<div class="toggle-row">
						<button class="proj-btn" class:active={!Id.Camera.IsOrtho} onclick={() => Id.Camera.IsOrtho = false}>PERSP</button>
						<button class="proj-btn" class:active={Id.Camera.IsOrtho}  onclick={() => Id.Camera.IsOrtho = true}>ORTHO</button>
					</div>
				</div>
				<div class="prop-row">
					<span class="prop-key">Near</span>
					<input class="num-input" type="number" bind:value={Id.Camera.Near} step="0.01" />
				</div>
				<div class="prop-row">
					<span class="prop-key">Far</span>
					<input class="num-input" type="number" bind:value={Id.Camera.Far} step="1" />
				</div>
				<div class="prop-row">
					<span class="prop-key">Aspect</span>
					<input class="num-input" type="number" bind:value={Id.Camera.Aspect} step="0.01" />
				</div>
				{#if !Id.Camera.IsOrtho}
					<div class="prop-row">
						<span class="prop-key">FOV</span>
						<input class="num-input" type="number" bind:value={Id.Camera.FOV} step="1" min="1" max="179" />
					</div>
				{/if}
				{#if Id.Camera.IsOrtho}
					<div class="subsection-label">ORTHO BOUNDS</div>
					<div class="prop-row">
						<span class="prop-key">Left</span>
						<input class="num-input" type="number" bind:value={Id.Camera.OrthoLeft} step="0.1" />
					</div>
					<div class="prop-row">
						<span class="prop-key">Right</span>
						<input class="num-input" type="number" bind:value={Id.Camera.OrthoRight} step="0.1" />
					</div>
					<div class="prop-row">
						<span class="prop-key">Top</span>
						<input class="num-input" type="number" bind:value={Id.Camera.OrthoTop} step="0.1" />
					</div>
					<div class="prop-row">
						<span class="prop-key">Bottom</span>
						<input class="num-input" type="number" bind:value={Id.Camera.OrthoBottom} step="0.1" />
					</div>
				{/if}
			</div>
		</div>
 
		<!-- Models -->
		<div class="section section-models">
			<div class="section-hdr">
				<span class="sh-icon">◼</span> MODELS
				<button class="btn-inline" onclick={AddMesh}>+ ADD MESH</button>
			</div>
			<div class="section-body models-list">
				{#each Id.Models as Model, i}
					<div class="model-card">
						<div class="model-card-top">
							<span class="mc-label">MODEL {i + 1}</span>
						</div>
						<div class="prop-row">
							<span class="prop-key">Mesh</span>
							<Dropdown Options={GetMeshes} bind:CurrentFilter={Model.MeshId} />
						</div>
						<div class="prop-row">
							<span class="prop-key">Animation</span>
							<Dropdown Options={GetAnimations} bind:CurrentFilter={Model.AnimationId} />
						</div>
						<div class="prop-row">
							<span class="prop-key">Texture</span>
							<Dropdown Options={GetTextures} bind:CurrentFilter={Model.TextureId} />
						</div>
						<div class="prop-row">
							<span class="prop-key">Shader</span>
							<Dropdown Options={GetShaders} bind:CurrentFilter={Model.ShaderId} />
						</div>
						<div class="divider"></div>
						<div class="prop-group">
							<div class="prop-label">Location</div>
							<Vector3Edit bind:vector={Model.Location} />
						</div>
						<div class="prop-group">
							<div class="prop-label">Rotation</div>
							<Vector3Edit bind:vector={Model.Rotation} />
						</div>
						<div class="prop-group">
							<div class="prop-label">Scale</div>
							<Vector3Edit bind:vector={Model.Scale} />
						</div>
					</div>
				{/each}
				{#if Id.Models.length === 0}
					<div class="models-empty">
						<span>No models — click ADD MESH above</span>
					</div>
				{/if}
			</div>
		</div>
 
		<!-- Output -->
		<div class="section">
			<div class="section-hdr">
				<span class="sh-icon">⬛</span> OUTPUT
			</div>
			<div class="section-body">
				<div class="prop-row">
					<span class="prop-key">Width</span>
					<input class="num-input" type="number" bind:value={Id.Width} step="1" min="1" />
				</div>
				<div class="prop-row">
					<span class="prop-key">Height</span>
					<input class="num-input" type="number" bind:value={Id.Height} step="1" min="1" />
				</div>
			</div>
		</div>
 
		<!-- Render -->
		<div class="render-bar">
			<button class="btn-render" onclick={Render}>
				<span>▶</span> RENDER SPRITE SHEET
			</button>
		</div>
 
	</div>
 
	<!-- ── Right: Preview ── -->
	<div class="preview-pane">
		<div class="preview-hdr">
			<span class="ph-ic">⬡</span>
			VIEWPORT
		</div>
		<div class="preview-body">
			<ScenePreview bind:this={View} SceneId={Id} />
		</div>
	</div>
 
</div>
 
<style>
	.scene-editor {
		display: flex;
		height: 100%;
		overflow: hidden;
	}
 
	.props {
		width: 284px;
		background: var(--bg-panel);
		border-right: 1px solid var(--border);
		display: flex;
		flex-direction: column;
		overflow-y: auto;
		flex-shrink: 0;
	}
 
	.props::-webkit-scrollbar          { width: 3px; }
	.props::-webkit-scrollbar-thumb    { background: var(--border-hi); }
 
	.section { border-bottom: 1px solid var(--border); }
	.section-models { flex: 1; }
 
	.section-hdr {
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
		position: sticky;
		top: 0;
		z-index: 2;
	}
 
	.sh-icon { color: var(--accent); font-size: 13px; }
 
	.section-body {
		padding: 8px;
		display: flex;
		flex-direction: column;
		gap: 6px;
	}
 
	.subsection-label {
		font-size: 9px;
		font-weight: 600;
		letter-spacing: 0.16em;
		color: var(--text-dim);
		padding: 4px 2px 0;
		border-top: 1px solid var(--border);
		margin-top: 2px;
	}
 
	.file-zone { position: relative; }
 
	.file-hidden {
		position: absolute;
		inset: 0;
		opacity: 0;
		cursor: pointer;
		width: 100%;
		height: 100%;
	}
 
	.file-label {
		display: flex;
		align-items: center;
		justify-content: center;
		gap: 7px;
		padding: 9px;
		border: 1px dashed var(--border-hi);
		color: var(--text-muted);
		font-size: 11px;
		cursor: pointer;
		letter-spacing: 0.06em;
		transition: border-color 0.15s, color 0.15s;
		font-family: 'Chakra Petch', sans-serif;
	}
 
	.file-label:hover { border-color: var(--accent); color: var(--accent); }
	.fl-icon          { font-size: 15px; }
 
	.btn {
		border: none;
		padding: 5px 10px;
		font-family: 'Chakra Petch', sans-serif;
		font-size: 10px;
		font-weight: 600;
		letter-spacing: 0.1em;
		cursor: pointer;
		display: flex;
		align-items: center;
		justify-content: center;
		gap: 6px;
		transition: background 0.15s, border-color 0.15s, color 0.15s;
	}
 
	.btn-ghost {
		background: var(--bg-raised);
		border: 1px solid var(--border-hi);
		color: var(--text-muted);
		width: 100%;
	}
 
	.btn-ghost:hover { border-color: var(--accent); color: var(--accent); background: var(--accent-dim); }
 
	.btn-inline {
		margin-left: auto;
		background: transparent;
		border: 1px solid var(--border-hi);
		color: var(--text-muted);
		padding: 2px 7px;
		font-family: 'Chakra Petch', sans-serif;
		font-size: 9px;
		font-weight: 600;
		letter-spacing: 0.1em;
		cursor: pointer;
		transition: border-color 0.15s, color 0.15s, background 0.15s;
	}
 
	.btn-inline:hover { border-color: var(--accent); color: var(--accent); background: var(--accent-dim); }
 
	.toggle-row {
		display: flex;
		gap: 2px;
		flex: 1;
	}
 
	.proj-btn {
		flex: 1;
		background: var(--bg-base);
		border: 1px solid var(--border-hi);
		color: var(--text-muted);
		padding: 4px 0;
		font-family: 'Chakra Petch', sans-serif;
		font-size: 9px;
		font-weight: 600;
		letter-spacing: 0.1em;
		cursor: pointer;
		transition: all 0.15s;
	}
 
	.proj-btn:hover       { border-color: var(--accent); color: var(--text); }
	.proj-btn.active      { background: var(--accent); border-color: var(--accent); color: #fff; }
 
	.num-input {
		flex: 1;
		background: var(--bg-base);
		border: 1px solid var(--border);
		color: var(--text);
		padding: 4px 7px;
		font-family: 'IBM Plex Mono', monospace;
		font-size: 11px;
		outline: none;
		width: 0;
		min-width: 0;
		transition: border-color 0.15s;
		text-align: right;
	}
 
	.num-input:focus { border-color: var(--accent); }
	.num-input::-webkit-inner-spin-button,
	.num-input::-webkit-outer-spin-button { opacity: 0.25; }
 
	.prop-group {
		display: flex;
		flex-direction: column;
		gap: 3px;
	}
 
	.prop-label {
		font-size: 10px;
		color: var(--text-muted);
		letter-spacing: 0.08em;
		text-transform: uppercase;
		padding-left: 2px;
	}
 
	.prop-row {
		display: flex;
		align-items: center;
		gap: 8px;
	}
 
	.prop-key {
		font-size: 11px;
		color: var(--text-muted);
		white-space: nowrap;
		min-width: 64px;
	}
 
	.models-list { gap: 8px; }
 
	.model-card {
		background: var(--bg-base);
		border: 1px solid var(--border);
		padding: 8px;
		display: flex;
		flex-direction: column;
		gap: 6px;
		transition: border-color 0.15s;
	}
 
	.model-card:hover { border-color: var(--border-hi); }
 
	.model-card-top {
		padding-bottom: 5px;
		border-bottom: 1px solid var(--border);
	}
 
	.mc-label {
		font-size: 9px;
		font-weight: 700;
		letter-spacing: 0.18em;
		color: var(--accent);
	}
 
	.divider {
		height: 1px;
		background: var(--border);
		margin: 2px 0;
	}
 
	.models-empty {
		color: var(--text-dim);
		font-size: 10px;
		padding: 16px;
		text-align: center;
		letter-spacing: 0.06em;
		line-height: 1.7;
	}
 
	.render-bar {
		padding: 10px 8px;
		background: #0e0e0e;
		border-top: 1px solid var(--border);
		flex-shrink: 0;
	}
 
	.btn-render {
		width: 100%;
		background: var(--accent);
		border: none;
		color: #fff;
		padding: 11px;
		font-family: 'Chakra Petch', sans-serif;
		font-size: 11px;
		font-weight: 700;
		letter-spacing: 0.16em;
		cursor: pointer;
		display: flex;
		align-items: center;
		justify-content: center;
		gap: 9px;
		transition: background 0.15s, box-shadow 0.15s;
		text-transform: uppercase;
	}
 
	.btn-render:hover {
		background: #f08848;
		box-shadow: 0 0 16px var(--accent-glow), 0 0 32px rgba(224,123,57,0.15);
	}
 
	.preview-pane {
		flex: 1;
		display: flex;
		flex-direction: column;
		overflow: hidden;
		background: var(--bg-base);
	}
 
	.preview-hdr {
		height: 30px;
		background: #0e0e0e;
		border-bottom: 1px solid var(--border);
		display: flex;
		align-items: center;
		gap: 7px;
		padding: 0 12px;
		font-size: 10px;
		font-weight: 600;
		letter-spacing: 0.16em;
		color: var(--text-muted);
		flex-shrink: 0;
	}
 
	.ph-ic { color: var(--accent); }
 
	.preview-body {
		flex: 1;
		display: flex;
		align-items: center;
		justify-content: center;
		overflow: hidden;
		position: relative;
		padding: 20px;
	}
 
	.preview-body::before {
		content: '';
		position: absolute;
		inset: 0;
		background-image:
			linear-gradient(var(--border) 1px, transparent 1px),
			linear-gradient(90deg, var(--border) 1px, transparent 1px);
		background-size: 28px 28px;
		opacity: 0.35;
		pointer-events: none;
	}
 
	.preview-body::after {
		content: '';
		position: absolute;
		top: 50%; left: 50%;
		transform: translate(-50%, -50%);
		width: 18px; height: 18px;
		border: 1px solid var(--border-hi);
		opacity: 0.5;
		pointer-events: none;
	}
</style>
