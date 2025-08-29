#ifndef interpolate_h_INCLUDED
#define interpolate_h_INCLUDED

float lerp(float a, float b, float t);

#ifdef CSM_BASE_IMPLEMENTATION

float lerp(float a, float b, float t)
{
	return (1.0f - t) * a + t * b;
}

#endif // CSM_BASE_INTERPOLATION

#endif // interpolate_h_INCLUDED
