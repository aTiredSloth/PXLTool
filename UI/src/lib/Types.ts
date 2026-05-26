type Vector3 =
{
	x: number;
	y: number;
	z: number;
};

type CameraData =
{
	Location: Vector3;
	Rotation: Vector3;

	IsOrtho: boolean;
	OrthoLeft: number;
	OrthoRight: number;
	OrthoTop: number;
	OrthoBottom: number;
	Aspect: number;
	Near: number;
	Far: number;
	FOV: number;
};

type Model =
{
	MeshId: number;
	AnimationId: number;
	ShaderId: number;
	Location: Vector3;
	Rotation: Vector3;
	Scale: Vector3;
};

type Scene =
{
	Name: string;
	Time: number;
	Camera: CameraData;
	Models: Model[];
};
