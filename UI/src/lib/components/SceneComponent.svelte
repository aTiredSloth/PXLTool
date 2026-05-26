<script lang="ts">
    import Dropdown from "./edit/Dropdown.svelte";
    import Vector3Edit from "./edit/Vector3Edit.svelte";
    import ScenePreview from "./ScenePreview.svelte";
    let {Id}: {Id:Scene} = $props();

    async function GetMeshes()
    {
    	let response = await fetch("api/meshes");

     	return response.json();
    }

    async function GetAnimations()
    {
    	let response = await fetch("api/animations");

     	return response.json();
    }

    async function GetShaders()
    {
    	let response = await fetch("api/shaders");

     	return response.json();
    }

    async function AddMesh()
    {
    	Id.Models.push();
    }
</script>

<div>

	<div>
		<Vector3Edit vector={Id.Camera.Location}></Vector3Edit>
		<Vector3Edit vector={Id.Camera.Rotation}></Vector3Edit>
	</div>
	<div>
		<button onclick={AddMesh}>Add Mesh</button>
		{#each Id.Models as Model}
			<div>
				Meshes
				<Dropdown Options={GetMeshes()} CurrentFilter={Model.MeshId}></Dropdown>
				Animations
				<Dropdown Options={GetAnimations()} CurrentFilter={Model.AnimationId}></Dropdown>
				Shaders
				<Dropdown Options={GetShaders()} CurrentFilter={Model.ShaderId}></Dropdown>
				<Vector3Edit vector={Model.Location}></Vector3Edit>
				<Vector3Edit vector={Model.Rotation}></Vector3Edit>
				<Vector3Edit vector={Model.Scale}></Vector3Edit>
			</div>
		{/each}
	</div>
	
<ScenePreview {Id}/>	
</div>
