#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
//    glm::vec3 a = glm::vec3(1,x);
//
//	return p0*p1;
    glm::vec3 a = (p0*(1-x)) + (x*p1);
//    return (p0*(1-x)) + (x*p1);
    return a;
	//! \todo Implement this function
	return glm::vec3();
}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{
    glm::vec3 d1 = ((-1*t*x + 2*t*x*x) - t*x*x*x) * p0;
    glm::vec3 d2 = (1+ (t-3)*x*x - (t-2)*x*x*x ) * p1;
    glm::vec3 d3 = (t*x + (3 - 2*t)*x*x + (t-2)*x*x*x) * p2;
    glm::vec3 d4 = (-t*x*x + t*x*x*x) * p3;
    //! \todo Implement this function
	return d1 + d2 + d3 + d4;
}
