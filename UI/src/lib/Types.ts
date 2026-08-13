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

type AssetID =
{
	Name: string;
	Id: number;
};

type Model =
{
	MeshId: AssetID;
	AnimationId: AssetID;
	TextureId: AssetID;
	ShaderId: AssetID;
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
	Width: number;
	Height: number;
	PostShaders: number[];
	FramesPerSecond: number;
};
