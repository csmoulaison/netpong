#ifndef interpolate_h_INCLUDED
#define interpolate_h_INCLUDED

f32 lerp(f32 a, f32 b, f32 t);

#ifdef CSM_BASE_IMPLEMENTATION

f32 lerp(f32 a, f32 b, f32 t)
{
	return (1.0f - t) * a + t * b;
}

#endif // CSM_BASE_INTERPOLATION

#endif // interpolate_h_INCLUDED
