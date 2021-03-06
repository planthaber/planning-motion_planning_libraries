#include "MotionPlanningLibrariesSbplMprimsVisualization.hpp"

#include <iostream>

#include <osg/Geometry>
#include <osg/Group>
#include <osg/ShapeDrawable>

using namespace vizkit3d;

struct MotionPlanningLibrariesSbplMprimsVisualization::Data {
    // Copy of the value given to updateDataIntern.
    //
    // Making a copy is required because of how OSG works
    motion_planning_libraries::SbplMotionPrimitives data;
};

// PUBLIC
MotionPlanningLibrariesSbplMprimsVisualization::MotionPlanningLibrariesSbplMprimsVisualization()
    : mAllAnglesShown(false), mRadiusEndpoints(0.05), p(new Data)
{
}

MotionPlanningLibrariesSbplMprimsVisualization::~MotionPlanningLibrariesSbplMprimsVisualization()
{
    delete p;
}

// PUBLIC SLOTS
bool MotionPlanningLibrariesSbplMprimsVisualization::allAnglesShown() const {
    return mAllAnglesShown;
}

void MotionPlanningLibrariesSbplMprimsVisualization::setShowAllAngles(bool enabled) {
    mAllAnglesShown = enabled;
    emit propertyChanged("show_all_angles");
}

double  MotionPlanningLibrariesSbplMprimsVisualization::getRadiusEndpoints() const {
    return mRadiusEndpoints;
}

void  MotionPlanningLibrariesSbplMprimsVisualization::setRadiusEndpoints(double radius) {
    mRadiusEndpoints = radius;
    emit propertyChanged("endpoint_radius_changed");
}

// PROTECTED
osg::ref_ptr<osg::Node> MotionPlanningLibrariesSbplMprimsVisualization::createMainNode()
{
    // Geode is a common node used for vizkit3d plugins. It allows to display
    // "arbitrary" geometries
    //return new osg::Geode();
    return new osg::Group();
}

void MotionPlanningLibrariesSbplMprimsVisualization::updateMainNode ( osg::Node* node )
{
    //osg::Geode* geode = static_cast<osg::Geode*>(node);
    osg::Group* group = node->asGroup();
    group->removeChildren(0, node->asGroup()->getNumChildren());
    
    // Update the main node using the data in        double sphere_radius = mRadiusEndpoints; p->data
    addPrimitives(group, p->data);
}

void MotionPlanningLibrariesSbplMprimsVisualization::updateDataIntern(motion_planning_libraries::SbplMotionPrimitives const& data)
{
    p->data = data;
    //std::cout << "got new sample data vector" << std::endl;
}

void MotionPlanningLibrariesSbplMprimsVisualization::addPrimitives(osg::Group* group, 
        motion_planning_libraries::SbplMotionPrimitives& primitives) {
        
    std::vector<motion_planning_libraries::Primitive> prim_list = primitives.mListPrimitives;
    std::vector<motion_planning_libraries::Primitive>::iterator it = prim_list.begin();
    
    int num_prims_per_angle = primitives.mListPrimitives.size() / primitives.mConfig.mNumAngles;
    double hue_step = 1.0 / (double)primitives.mConfig.mNumAngles;
    double hue = 0.0;
    float r=0, g=0, b=0;
    osg::Vec4 color(r, g, b, 1.0f); 
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
    
    for(int i=0; it != prim_list.end(); it++, i++) {
        
        // Take a different color for each set of primitives.
        //std::cout << "Draw primitive " << i << std::endl;
        if(i >= num_prims_per_angle) {
            if(!mAllAnglesShown) {
                break;
            }
            i = 0;
        }
        
        if(i == 0) {            
            vizkit3d::hslToRgb(hue, 1.0, 0.5, r, g, b);
            color = osg::Vec4(r, g, b, 1.0f);    
            colors = new osg::Vec4Array;
            colors->clear();
            colors->push_back( color );
            hue += hue_step;
            //std::cout << "Change color" << std::endl;
        }        
        
        //std::cout << "Set color to RGB: " << r << " " << g << " " << b << std::endl;
    
        osg::ref_ptr<osg::Geode> geode = new osg::Geode();
        
        if(mRadiusEndpoints > 0) {
            // Create sphere.
            osg::ref_ptr<osg::Sphere> sp = new osg::Sphere(osg::Vec3d(0,0,0), mRadiusEndpoints);
            osg::ref_ptr<osg::ShapeDrawable> sd = new osg::ShapeDrawable(sp.get());
            sd->setColor(color);
            geode->addDrawable(sd.get());
            
            // Create triangle.
            osg::ref_ptr<osg::Geometry> triangle_geometry = new osg::Geometry();
            osg::ref_ptr<osg::Vec3Array> triangle_vertices = new osg::Vec3Array();
            triangle_vertices->push_back(osg::Vec3(0.0, 2*mRadiusEndpoints, 0));
            triangle_vertices->push_back(osg::Vec3(4*2*mRadiusEndpoints, 0.0, 0));
            triangle_vertices->push_back(osg::Vec3(0.0, -2*mRadiusEndpoints, 0));
            triangle_geometry->setVertexArray(triangle_vertices);
            osg::ref_ptr<osg::DrawElementsUInt> triangle_face = 
                    new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
            triangle_face->push_back(0);
            triangle_face->push_back(1);
            triangle_face->push_back(2);
            triangle_geometry->addPrimitiveSet(triangle_face);
            triangle_geometry->setColorArray(colors);
            triangle_geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
            geode->addDrawable(triangle_geometry);
            
            // Move the sphere and the triangle to the endpose (converted from grid to world)
            // using a transform.
            base::Vector3d mEndPose = it->mEndPose;
            //std::cout << "Endpose " << mEndPose[0] << " " << mEndPose[1] << " scale factor " << primitives.mScaleFactor << std::endl;
            double x = mEndPose[0] * primitives.mConfig.mGridSize;
            double y = mEndPose[1] * primitives.mConfig.mGridSize;
            double z = 0; // mEndPose[2] * primitives.mScaleFactor; // z is discrete orientation
            double theta = mEndPose[2] * ((2*M_PI)/primitives.mConfig.mNumAngles);

            //std::cout << "Draw sphere and triangle at XYZTHETA: " << x << " "  << y << " " << z << " " << theta << " " << std::endl; 
            
            osg::ref_ptr<osg::PositionAttitudeTransform> transform = 
                    new osg::PositionAttitudeTransform();
            osg::Vec3 position = osg::Vec3d(x, y, z);
            position[2] += 0.01; // Moves the waypoints a little bit above the z=0 plane.
            transform->setPosition(position);
            // osg::Quat(0,0,1,heading) != osg::Quat(heading, Vec(0,0,1)).. why?
            transform->setAttitude(osg::Quat(theta, osg::Vec3f(0,0,1)));
            transform->addChild(geode);
        
            // Add transformer containing the spehere and the triangle to the passed group.
            group->addChild(transform);
        }
        
        // Draw intermediate lines representing the mprim within the world.
        osg::ref_ptr<osg::Geode> geode_intermediate_points = new osg::Geode();
        
        std::vector<base::Vector3d> i_p = it->mIntermediatePoses;
        osg::ref_ptr<osg::Geometry> fp_geometry = new osg::Geometry();
        osg::ref_ptr<osg::Vec3Array> fp_vertices = new osg::Vec3Array();
        base::Vector3d v_tmp;
        //std::cout << "Number of intermediate points: " << i_p.size() << std::endl;
        for(unsigned int i = 0; i < i_p.size(); ++i) {
            v_tmp = i_p[i];
            fp_vertices->push_back(osg::Vec3(v_tmp[0], v_tmp[1], 0));
            if(i == i_p.size() - 1) {
                //std::cout << "Draw intermediate points to endpoint XY: " << v_tmp[0] << " " << v_tmp[1] << std::endl;
            }
        }
        
        fp_geometry->setVertexArray(fp_vertices);
        fp_geometry->setColorArray(colors);
        fp_geometry->setColorBinding(osg::Geometry::BIND_OVERALL);
        fp_geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP,0,i_p.size()));
        geode_intermediate_points->addDrawable(fp_geometry);
        
        // Add intermediate points to the passed group.
        group->addChild(geode_intermediate_points);
    }
}

//Macro that makes this plugin loadable in ruby, this is optional.
VizkitQtPlugin(MotionPlanningLibrariesSbplMprimsVisualization)

