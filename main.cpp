#include <iostream>
#include <cmath>
#include <vector>
#include <fstream>
#include "vec3.h"
#include "object.h"
#include <algorithm>
using namespace std;

#define HEIGHT 480
#define WIDTH 640
#define RECURSION_DEPTH 5



Vec3 sendRay(Vec3 rayOrigin, Vec3 rayDirection, int level, vector<Object*> objects, vector<Vec3> lights){
	double intersectPoint = INFINITY;
	bool doesIntersect = false;
	Object* intersectObject = NULL;
	for(vector<Object*>::iterator it = objects.begin() ; it != objects.end(); ++it) {
		vector<double> points = (*it)->intersectionPoints(rayOrigin, rayDirection);
		if(points.size()==1){
			if(points[0]<0) continue;
			else{
				doesIntersect = true;
				if(intersectPoint > points[0]){
					intersectPoint = points[0];
					intersectObject = (*it);
				}
			}
		} else if(points.size()==2){

			if(points[0]>points[1])
				swap(points[0],points[1]);

			if(points[0]>0){
				doesIntersect = true;
				if(intersectPoint > points[0]){
					intersectPoint = points[0];
					intersectObject = (*it);
				}
			} else if(points[1]>0){
				doesIntersect = true;
				if(intersectPoint > points[1]){
					intersectPoint = points[1];
					intersectObject = (*it);
				}
			}
		}
	}

	if(!doesIntersect) return Vec3(2);
	Vec3 point = rayOrigin + rayDirection * intersectPoint;
	Vec3 normal = intersectObject->getNormal(point);
	Vec3 finalColor(0);

	cout<<"Normal"<<rayDirection.dot(normal)<<endl;
	bool isIn = false;
	if(normal.dot(rayDirection)>0) {
		normal = normal * -1;
		isIn = true;
		cout<<"It is in"<<endl;
	}

	if(intersectObject->objectType == SPECULAR && level < RECURSION_DEPTH) {
		Vec3 reflectionDir = rayDirection - (normal * 2 * rayDirection.dot(normal));
		reflectionDir.normalize();
		// cout<<"Reflection "<<level<<endl;
		// point.print();
		// reflectionDir.print();
		// cout<<reflectionDir.dot(normal)<<endl;
		// cout<<endl;
		double facingratio = -1 * rayDirection.dot(normal);
		double fresneleffect = 0.1 + pow(1- facingratio, 3)*0.9;
		Vec3 reflectionColor = sendRay(point + normal * 0.01 , reflectionDir, level + 1, objects, lights);

		Vec3 refractionColor(0);
		if(intersectObject->transparency > 0.0){
			double n1 = 1;
			double n2 = intersectObject->transparency;
			double n = isIn ? n2/n1 : n1/n2;
			double c1 = -1 * rayDirection.dot(normal);
			double c2 = sqrt(1 - (n*n * (1 - c1*c1)));

			Vec3 refractionDir = (rayDirection * n) + (normal * (n*c1 - c2));
			refractionDir.normalize();
			refractionColor = sendRay(point - normal * 0.01, refractionDir, level + 1, objects, lights);
		}
		cout<<"Dda"<<fresneleffect<<endl;
		finalColor = (reflectionColor * fresneleffect + refractionColor * (1 - fresneleffect)) * intersectObject->surfaceColor;

	}

	else{
		for(vector<Vec3>::iterator light = lights.begin() ; light != lights.end(); ++light) {
			Vec3 lightDir = point - (*light);
			bool shadow = false;
			intersectPoint = INFINITY;
			for(vector<Object*>::iterator it = objects.begin() ; it != objects.end(); ++it){
				vector<double> points = (*it)->intersectionPoints(*light, lightDir);
				if(points.size()==1){
					if(points[0]<0) continue;
					else{
						shadow = true;
						if(intersectPoint > points[0]){
							intersectPoint = points[0];
						}
					}
				} else if(points.size()==2){
					if(points[0]>points[1])
						swap(points[0],points[1]);

					if(points[0]>0){
						shadow = true;
						if(intersectPoint > points[0]){
							intersectPoint = points[0];
						}
					} else if(points[1]>0){
						shadow = true;
						if(intersectPoint > points[1]){
							intersectPoint = points[1];
						}
					}
				}
			}
			if(point == (*light + lightDir * intersectPoint)) shadow = false;
			if(!shadow) {
				finalColor += intersectObject->surfaceColor * max(0., -1*normal.dot(lightDir));
				cout<<"No shadow"<<endl;
			}
		}
	}
	return finalColor;
}

void init(vector<Object*> objects, vector<Vec3> lights){
	double fov = 30, aspectRatio = WIDTH/(double) HEIGHT;
	double angle = tan((M_PI/2)*fov/180);

	vector<vector<Vec3> > image(WIDTH, vector<Vec3>(HEIGHT, Vec3()));
	for(int i=0;i<WIDTH;i++){
		for(int j=0;j<HEIGHT;j++){
			double xx = (2 * ((i + 0.5) / WIDTH) - 1) * angle * aspectRatio;
			double yy = (1 - 2 * ((j + 0.5) / HEIGHT)) * angle;
			Vec3 rayDirection(xx, yy, -1);
			rayDirection.normalize();
			image[i][j] = sendRay(Vec3(), rayDirection, 0, objects, lights);
		}
	}

	ofstream ofs("./image.ppm", ios::out);
	ofs << "P3\n" << WIDTH << " " << HEIGHT << "\n255\n";
	for(int j=0;j<HEIGHT;j++) {
		for(int i=0;i<WIDTH;i++){
			// cout<<image[i][j].x<<" "<<image[i][j].y<<" "<<image[i][j].z<<endl;
			ofs<<int(min(1., image[i][j].x)*255)<<" ";
			ofs<<int(min(1., image[i][j].y)*255)<<" ";
			ofs<<int(min(1., image[i][j].z)*255)<<" ";
		}
		ofs<<endl;
	}
	ofs.close();
}
int main(){
	vector<Object*> objects;
	vector<Vec3> lights;
	Sphere sp1(Vec3(0,10,-55), 1, Vec3(1,0,0), 1.1, SPECULAR);
	Sphere sp2(Vec3(0,5,-50), 1, Vec3(1,1,0), 1.0, SPECULAR);
	Sphere sp3(Vec3(0,0,-55), 2, Vec3(0,1,1), 1.1, DIFFUSED);
	Sphere sp4(Vec3(0, -5, -60),3, Vec3(1,0,1), 1.2, SPECULAR);
	Sphere sp5(Vec3(0,-10,-55), 1, Vec3(0,0,1), 1.3, DIFFUSED);
	objects.push_back(&sp1);
	objects.push_back(&sp2);
	objects.push_back(&sp3);
	objects.push_back(&sp4);
	objects.push_back(&sp5);
	lights.push_back(Vec3(10,50,-10));
	init(objects, lights);
}