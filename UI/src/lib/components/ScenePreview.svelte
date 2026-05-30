<script lang="ts">

	let {SceneId}: {SceneId:Scene} = $props();
	let image = $state<string | null>(null);
	export async function reload()
	{
  		const response = await fetch('/api/scene/'+ SceneId.Name +'/render');
   	const data = await response.json();
    
    	const colorUrl = `data:image/png;base64,${data.color}`;
    	const normalUrl = `data:image/png;base64,${data.normal}`;
    	const depthUrl = `data:image/png;base64,${data.depth}`;
     	image = colorUrl;
	}
</script>

<div class="preview">
	{#if image}
		<img class="render-img" src={image} alt="Sprite Sheet Render" />
		<div class="img-overlay">
			<span class="overlay-label">SPRITE SHEET OUTPUT</span>
		</div>
	{:else}
		<div class="empty">
			<div class="empty-reticle">
				<div class="ret-h"></div>
				<div class="ret-v"></div>
				<div class="ret-dot"></div>
			</div>
			<div class="empty-sub">Output will appear here</div>
		</div>
	{/if}
</div>
 
<style>
	.preview {
		position: relative;
		width: 100%;
		height: 100%;
		display: flex;
		align-items: center;
		justify-content: center;
		z-index: 1;
	}
 
	/* Render output */
	.render-img {
		max-width: 100%;
		max-height: 100%;

		width: auto;
		height: auto;

		object-fit: contain;
		image-rendering: pixelated;

		border: 1px solid var(--border-hi);

		box-shadow:
			0 0 0 1px var(--border),
			0 8px 40px rgba(0,0,0,0.7);

		z-index: 1;
	}
 
	.img-overlay {
		position: absolute;
		bottom: 20px;
		left: 50%;
		transform: translateX(-50%);
		background: rgba(0,0,0,0.6);
		border: 1px solid var(--border-hi);
		padding: 3px 10px;
		font-size: 9px;
		letter-spacing: 0.16em;
		color: var(--text-muted);
		z-index: 2;
		backdrop-filter: blur(4px);
	}
 
	/* Empty state */
	.empty {
		display: flex;
		flex-direction: column;
		align-items: center;
		gap: 14px;
	}
 
	.empty-reticle {
		position: relative;
		width: 52px;
		height: 52px;
		opacity: 0.3;
	}
 
	.ret-h {
		position: absolute;
		top: 50%; left: 0; right: 0;
		height: 1px;
		background: var(--text-muted);
		transform: translateY(-50%);
	}
 
	.ret-v {
		position: absolute;
		left: 50%; top: 0; bottom: 0;
		width: 1px;
		background: var(--text-muted);
		transform: translateX(-50%);
	}
 
	.ret-dot {
		position: absolute;
		top: 50%; left: 50%;
		transform: translate(-50%, -50%);
		width: 6px; height: 6px;
		border: 1px solid var(--text-muted);
		border-radius: 50%;
	}
 
	.empty-text {
		color: var(--text-muted);
		font-size: 11px;
		letter-spacing: 0.07em;
		font-family: 'Chakra Petch', sans-serif;
	}
 
	.empty-sub {
		color: var(--text-dim);
		font-size: 10px;
		letter-spacing: 0.08em;
		font-family: 'Chakra Petch', sans-serif;
	}
</style>
