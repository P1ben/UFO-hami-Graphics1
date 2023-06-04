#include "framework.h"

// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char * const vertexSource = R"(
	#version 330				// Shader 3.3
	precision highp float;		// normal floats, makes no difference on desktop computers

	uniform mat4 MVP;			// uniform variable, the Model-View-Projection transformation matrix
	layout(location = 0) in vec2 vp;	// Varying input: vp = vertex position is expected in attrib array 0

	void main() {
		gl_Position = vec4(vp.x, vp.y, 0, 1) * MVP;		// transform vp from modeling space to normalized device space
	}
)";

// fragment shader in GLSL
const char * const fragmentSource = R"(
	#version 330			// Shader 3.3
	precision highp float;	// normal floats, makes no difference on desktop computers
	
	uniform vec3 color;		// uniform variable, the color of the primitive
	out vec4 outColor;		// computed color of the current pixel

	void main() {
		outColor = vec4(color, 1);	// computed color is the color of the primitive
	}
)";

const int db = 100;
GPUProgram gpuProgram; // vertex and fragment shaders

struct Color {
	float r;
	float g;
	float b;
	float a;
};

class Circle {
private:
	float radius = 0;
	float pos_x = 0;
	float pos_y = 0;
	unsigned int obj_id = 0;
	Color color = {0, 0, 0, 0};
	vec2 offset = 0;
public:
	Circle() {}
	Circle(float rad, float pos_x, float pos_y, Color color) :radius(rad), pos_x(pos_x), pos_y(pos_y), color(color), offset({0, 0}) {
		glGenBuffers(1, &this->obj_id);
		glBindBuffer(GL_ARRAY_BUFFER, this->obj_id);

		float pi_2 = M_PI * 2.0f;
		float vertices[2 * db + 4];

		vertices[0] = this->pos_x;
		vertices[1] = this->pos_y;

		for (int i = 1; i <= db; i++) {
			vertices[2 * (i)] = this->pos_x + radius * cos(pi_2 / db * i);
			vertices[2 * (i)+1] = this->pos_y + radius * sin(pi_2 / db * i);
		}
		vertices[2 * db + 2] = vertices[2];
		vertices[2 * db + 3] = vertices[3];

		glBufferData(GL_ARRAY_BUFFER,
			sizeof(vertices),  
			vertices,
			GL_STATIC_DRAW);	

		glEnableVertexAttribArray(0);  
	}
	void Draw() {
		int location = glGetUniformLocation(gpuProgram.getId(), "color");
		glUniform3f(location, color.r, color.g, color.b); 
		glBindBuffer(GL_ARRAY_BUFFER, this->obj_id);
		glVertexAttribPointer(0,       
			2, GL_FLOAT, GL_FALSE, 
			0, NULL); 		     
		glDrawArrays(GL_TRIANGLE_FAN, 0, db + 2);
	}
	unsigned int GetObjectId() {
		return this->obj_id;
	}
	void Refresh() {
		glBindBuffer(GL_ARRAY_BUFFER, this->obj_id);
		float pi_2 = M_PI * 2.0f;

		float vertices[2 * db + 4];

		vertices[0] = this->pos_x + offset.x;
		vertices[1] = this->pos_y + offset.y;

		for (int i = 1; i <= db; i++) {
			vertices[2 * (i)] = this->pos_x + radius * cos(pi_2 / db * i) + offset.x;
			vertices[2 * (i)+1] = this->pos_y + radius * sin(pi_2 / db * i) + offset.y;
		}
		vertices[2 * db + 2] = vertices[2];
		vertices[2 * db + 3] = vertices[3];

		glBufferData(GL_ARRAY_BUFFER, 	// Copy to GPU target
			sizeof(vertices),  // # bytes
			vertices,	      	// address
			GL_STATIC_DRAW);	// we do not change later
	}
	void SetCoords(vec2 coords) {
		this->pos_x = coords.x;
		this->pos_y = coords.y;
		Refresh();
	}
	void SetCoords(float x, float y) {
		this->pos_x = x;
		this->pos_y = y;
		Refresh();
	}
	vec2 getCoords() {
		return vec2(pos_x, pos_y);
	}
	void SetOffset(vec2 offset) {
		this->offset = offset;
		Refresh();
	}
};
class Trail {
private:
	unsigned int obj_id;
	std::vector<vec2> points;
public:
	Trail() : points(std::vector<vec2>()), obj_id(0) {};
	void AddPoint(vec2 point) {
		if(obj_id == 0) glGenBuffers(1, &this->obj_id);
		points.push_back(point);
		glBindBuffer(GL_ARRAY_BUFFER, this->obj_id);
		glBufferData(GL_ARRAY_BUFFER, 	// Copy to GPU target
			sizeof(vec2) * points.size(),  // # bytes
			points.data(),	      	// address
			GL_STATIC_DRAW);
	}
	void Draw() {
		int location = glGetUniformLocation(gpuProgram.getId(), "color");
		glUniform3f(location, 1.0f, 1.0f, 1.0f);
		glBindBuffer(GL_ARRAY_BUFFER, this->obj_id);
		glVertexAttribPointer(0,       // vbo -> AttribArray 0
			2, GL_FLOAT, GL_FALSE, // two floats/attrib, not fixed-point
			0, NULL); 		     // stride, offset: tightly packed
		glDrawArrays(GL_LINE_STRIP, 0, points.size());
	}
};
class Hyp {
	vec3 point;
	vec3 v0;
public:
	Hyp(vec2 point, vec3 v0) : v0(v0) {
		this->point = ProjectPoincToHyp(point);
	}
	Hyp(vec3 point, vec3 v0) : point(point), v0(v0){ 
	}
	vec3 ProjectPoincToHyp(vec2 point) {
		return vec3(point.x, point.y, 1) / sqrtf(1 - point.x * point.x - point.y * point.y);
	}
	vec2 ProjectHypToPoinc(vec3 point) {
		return vec2( point.x / (point.z + 1),  point.y / (point.z + 1));
	} 
	float len(vec3 v) {
		return sqrt(v.x * v.x + v.y * v.y - v.z * v.z);
	}
	void SetPoint(vec2 dist) {
		this->point = ProjectPoincToHyp(dist);
	}
	vec2 ReturnPoinc() {
		return this->ProjectHypToPoinc(point);
	}
	float Dot(vec3 a, vec3 b) {
		return a.x * b.x + a.y * b.y - a.z * b.z;
	}
	vec3 NormalizeHyp(vec3 v) {
		return v * (1 / len(v));
	}
	float GetW() {
		return point.z;
	}

	// 1. feladat: Meroleges
	inline vec3 Cross(const vec3& v1, const vec3& v2) {
		return vec3(v1.y * -v2.z - -v1.z * v2.y, -v1.z * v2.x - v1.x * -v2.z, v1.x * v2.y - v1.y * v2.x);
	}

	// 2. feladat: Adott pontbol �s sebess�g vektorral indulo pont helyenek �s sebesseg vektoranak szamitasa t idovel kesobb.
	Hyp NewPoint(float t) {
		vec3 q = this->point * coshf(t) + v0 * sinhf(t);
		vec3 vq = this->point * sinhf(t) + v0 * coshf(t);
		CorrectPAV0(&q, &vq);
		Hyp q_point = Hyp(q, NormalizeHyp(vq));
		return q_point;
	}

	// 3. feladat: Egy ponthoz kepest egy masik pont iranyanak es tavolsaganak meghatarozasa.
	float GetDistanceDirection(Hyp q, vec3* direction) {
		float dist = acoshf(Dot(-q.point, this->point));
		*direction = (q.point - (this->point * coshf(dist))) / sinhf(dist);
		return dist;
	}

	// 4. feladat: Egy ponthoz kepest adott iranyban es tavolsagra levo pont eloallitasa.
	Hyp CreateNewPointDelta(float dist, float rot) {
		Hyp new_point = *this;
		new_point.RotateV0(rot);
		new_point = new_point.NewPoint(dist);
		return new_point;
	}

	// 5. feladat: Egy pontban egy vektor elforgatasa adott szoggel.
	vec3 RotateVector(vec3 v, vec3 p, float rot) {
		float v_len = len(v);
		v = NormalizeHyp(v);
		vec3 v_n = v * cosf(rot) + NormalizeHyp(Cross(NormalizeHyp(v), p)) * sinf(rot);
		return NormalizeHyp(v_n);
	}

	// 6. feladat: Egy kozelito pont es sebessegvektorhoz a geometria szabalyait teljesito, kozeli pont es sebesseg valasztasa.
	void CorrectPAV0(vec3* p, vec3* v) {
		*p = *p * PointLambda(*p);
		*v = *v + VectorLambda(*v, *p) * *p;
	}

	float VectorLambda(vec3 v, vec3 p) {
		float a = Dot(v, p);
		float b = p.z * p.z - p.x * p.x - p.y * p.y;
		return a / b;
	}
	float PointLambda(vec3 p) {
		return sqrtf(-1 / Dot(p, p));
	}
	vec2 getRotationVector() {
		return normalize(vec2(v0.x, v0.y));
	}
	void RotateV0(float rot) {
		v0 = RotateVector(v0, point, rot);
		v0 = v0 + point * VectorLambda(v0, point);
		v0 = NormalizeHyp(v0);
	}
	void SetV0(vec3 v) {
		this->v0 = v;
	}
};
class HypCircle {
private:
	float radius = 0;
	Hyp pos;
	unsigned int obj_id = 0;
	Color color = {0, 0, 0, 0};
public:
	HypCircle() : pos(Hyp({ 0, 0, 1 }, { 1, 0, 0 })), color({0, 0, 0, 0}), radius(0) {}
	HypCircle(float def_rad, Hyp pos, Color color) :radius(def_rad), pos(pos), color(color) {
		vec2 pos_poin = pos.ReturnPoinc();
		glGenBuffers(1, &this->obj_id);	// Generate 1 buffer
		glBindBuffer(GL_ARRAY_BUFFER, this->obj_id);
		float pi_2 = M_PI * 2.0f;

		float vertices[2 * db + 4];

		vertices[0] = pos_poin.x;
		vertices[1] = pos_poin.y;
		Hyp copy = pos;
		for (int i = 1; i <= db; i++) {
			Hyp temp = copy.CreateNewPointDelta(radius, pi_2 / db * i);
			vertices[2 * (i)] = temp.ReturnPoinc().x;
			vertices[2 * (i)+1] = temp.ReturnPoinc().y;
		}
		vertices[2 * db + 2] = vertices[2];
		vertices[2 * db + 3] = vertices[3];

		glBufferData(GL_ARRAY_BUFFER, 	// Copy to GPU target
			sizeof(vertices),  // # bytes
			vertices,	      	// address
			GL_STATIC_DRAW);	// we do not change later

		glEnableVertexAttribArray(0);  // AttribArray 0
	}
	Hyp GetPos() {
		return pos;
	}
	void SetPos(Hyp a) {
		pos = a;
	}
	float GetRadius() {
		return radius;
	}
	void Draw() {
		Refresh();
		int location = glGetUniformLocation(gpuProgram.getId(), "color");
		glUniform3f(location, color.r, color.g, color.b); // 3 floats
		glBindBuffer(GL_ARRAY_BUFFER, this->obj_id);
		glVertexAttribPointer(0,       // vbo -> AttribArray 0
			2, GL_FLOAT, GL_FALSE, // two floats/attrib, not fixed-point
			0, NULL); 		     // stride, offset: tightly packed
		glDrawArrays(GL_TRIANGLE_FAN, 0, db + 2);
	}
	unsigned int GetObjectId() {
		return this->obj_id;
	}
	void Refresh() {
		vec2 pos_poin = pos.ReturnPoinc();

		glBindBuffer(GL_ARRAY_BUFFER, this->obj_id);
		float pi_2 = M_PI * 2.0f;

		float vertices[2 * db + 4];

		vertices[0] = pos_poin.x;
		vertices[1] = pos_poin.y;

		Hyp copy = pos;
		for (int i = 1; i <= db; i++) {
			Hyp temp = copy.CreateNewPointDelta(radius, pi_2 / db * i);
			vertices[2 * (i)] = temp.ReturnPoinc().x;//pos_poin.x + radius * cos(pi_2 / db * i);
			vertices[2 * (i)+1] = temp.ReturnPoinc().y;//pos_poin.y + radius * sin(pi_2 / db * i);
		}
		vertices[2 * db + 2] = vertices[2];
		vertices[2 * db + 3] = vertices[3];

		glBufferData(GL_ARRAY_BUFFER, 	// Copy to GPU target
			sizeof(vertices),  // # bytes
			vertices,	      	// address
			GL_STATIC_DRAW);	// we do not change later
	}
	vec2 getCoords() {
		vec2 p = pos.ReturnPoinc();
		return p;
	}
	void SetRadius(float rad) {
		this->radius = rad;
	}
	void MoveForward(float amount) {
		pos = pos.NewPoint(amount);
	}
	void MoveBackwards(float amount) {
		pos = pos.NewPoint(-amount);
	}
	void RotateRight(float amount) {
		pos.RotateV0(-amount);
	}
	void RotateLeft(float amount) {
		pos.RotateV0(amount);
	}
};
class UFO {
private:
	HypCircle body;
	HypCircle eyes[4];
	HypCircle mouth;
	Color color;
	bool mouth_direction = true;
	UFO* other;
	Trail trail;
public:
	UFO() : color({0, 0, 0}), other(nullptr) {}
	UFO(Color col) : color(col), other(nullptr) {
		body = HypCircle(0.2f, Hyp({ 0, 0, 1 }, { 0, 1, 0 }), color);

		// Create eyes
		eyes[0] = HypCircle(0.05f, Hyp({ 0, 0, 1 }, { 0, 1, 0 }), Color({ 1, 1, 1, 1 })); 
		eyes[1] = HypCircle(0.05f, Hyp({ 0, 0, 1 }, { 0, 1, 0 }), Color({ 1, 1, 1, 1 })); 
		eyes[2] = HypCircle(0.03f, Hyp({ 0, 0, 1 }, { 0, 1, 0 }), Color({ 0, 0, 1, 1 })); 
		eyes[3] = HypCircle(0.03f, Hyp({ 0, 0, 1 }, { 0, 1, 0 }), Color({ 0, 0, 1, 1 })); 

		trail = Trail();

		mouth = HypCircle(0.1f, Hyp({ 0, 0, 1 }, { 0, 1, 0 }), Color({ 0, 0, 0, 1 }));

	}
	void SetOther(UFO* ot) {
		other = ot;
	}
	void Draw() {
		Realign();
		trail.Draw();
		body.Draw();
		mouth.Draw();
		for (int i = 0; i < 4; i++) {
			eyes[i].Draw();
		}
	}
	void MoveMouth() {
		float delta = 0.005f;
		if (mouth_direction) {
			mouth.SetRadius(mouth.GetRadius() - delta);
		}
		else {
			mouth.SetRadius(mouth.GetRadius() + delta);
		}
		if (mouth.GetRadius() > 0.1 || mouth.GetRadius() < 0) mouth_direction = !mouth_direction;
	}
	void Realign() {
		float eye_dir = 0.6;

		Hyp copy = body.GetPos();

		copy.SetV0(copy.getRotationVector());
		copy.RotateV0(eye_dir);
		Hyp left_eye = copy;
		left_eye = left_eye.NewPoint(body.GetRadius());
		eyes[0].SetPos(left_eye);

		vec3 dir;
		left_eye.GetDistanceDirection(other->body.GetPos(), &dir);
		left_eye.SetV0(dir);
		left_eye = left_eye.NewPoint(eyes[0].GetRadius() - eyes[2].GetRadius() / 2);
		eyes[2].SetPos(left_eye);

		copy.RotateV0(-eye_dir * 2);
		Hyp right_eye = copy;
		right_eye = right_eye.NewPoint(body.GetRadius());
		eyes[1].SetPos(right_eye);

		right_eye.GetDistanceDirection(other->body.GetPos(), &dir);
		right_eye.SetV0(dir);
		right_eye = right_eye.NewPoint(eyes[1].GetRadius() - eyes[3].GetRadius() / 2);
		eyes[3].SetPos(right_eye);

		for (int i = 0; i < 4; i++) {
			eyes[i].Refresh();
		}
		copy.RotateV0(eye_dir);
		Hyp mouth_point = copy.NewPoint(body.GetRadius());
		mouth.SetPos(mouth_point);
		mouth.Refresh();
	}
	void RotateRight(float rot) {
		body.RotateRight(rot);
	}
	void RotateLeft(float rot) {
		body.RotateLeft(rot);
	}
	void MoveForward(float amount) {
		body.MoveForward(amount);
	}
	void MoveBackwards(float amount) {
		body.MoveBackwards(amount);
	}
	void TrailAppendPoint() {
		trail.AddPoint(this->body.getCoords());
	}
};

unsigned int vao;	   // virtual world on the GPU
unsigned int vbo;


Circle background;
UFO first;
UFO second;
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);

	glGenVertexArrays(1, &vao);	// get 1 vao id
	glBindVertexArray(vao);		// make it active

	background = Circle(1, 0, 0, { 0, 0.0f, 0.0f, 1 });

	first = UFO({ 1.0f, 0, 0, 1 });
	second = UFO({ 0, 1.0f, 0, 0 });
	first.SetOther(&second);
	second.SetOther(&first);
	second.RotateRight(M_PI / 2.0f);
	second.MoveForward(1);


	gpuProgram.create(vertexSource, fragmentSource, "outColor");
}

// Window has become invalid: Redraw
float rot = 0;
void onDisplay() {
	glClearColor(0.5f, 0.5f, 0.5f, 0);     // background color
	glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer

	glEnableVertexAttribArray(0);  // AttribArray 0
	int location = glGetUniformLocation(gpuProgram.getId(), "color");
	glUniform3f(location, 1.0f, 0.0f, 0.0f); // 3 floats

	float MVPtransf[4][4] = { 1, 0, 0, 0,    // MVP matrix, 
							  0, 1, 0, 0,    // row-major!
							  0, 0, 1, 0,
							  0, 0, 0, 1 };

	location = glGetUniformLocation(gpuProgram.getId(), "MVP");	// Get the GPU location of uniform variable MVP
	glUniformMatrix4fv(location, 1, GL_TRUE, &MVPtransf[0][0]);	// Load a 4x4 row-major float matrix to the specified location

	background.Draw();
	first.Draw();
	second.Draw();
	glutSwapBuffers(); // exchange buffers for double buffering
}
bool pressed[256] = { false, };
// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	pressed[key] = true;
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
	pressed[key] = false;
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;
	printf("Mouse moved to (%3.2f, %3.2f)\n", cX, cY);
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;

	char * buttonStat;
	switch (state) {
	case GLUT_DOWN: buttonStat = "pressed"; break;
	case GLUT_UP:   buttonStat = "released"; break;
	}

	switch (button) {
	case GLUT_LEFT_BUTTON:   printf("Left button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);   break;
	case GLUT_MIDDLE_BUTTON: printf("Middle button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY); break;
	case GLUT_RIGHT_BUTTON:  printf("Right button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);  break;
	}
}

long elapsed_time = 0;
const int framerate = 60;
const float move_speed = 0.02f;
const float rot_speed = 0.05f;

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long curr_time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
	if (curr_time - elapsed_time > (int)(1000 / framerate)) {
		for (int i = 0; i < (int)((curr_time - elapsed_time) / (1000 / framerate)); i++) {
			if (pressed['e']) {
				first.MoveForward(move_speed);
				first.TrailAppendPoint();
			}
			if (pressed['s']) {
				first.RotateRight(rot_speed);
			}
			if (pressed['f']) {
				first.RotateLeft(rot_speed);
			}
			if (pressed['d']) {
				first.MoveBackwards(move_speed);
				first.TrailAppendPoint();
			}
			second.RotateRight(rot_speed);
			second.MoveForward(move_speed);
			second.TrailAppendPoint();
			first.MoveMouth();
			second.MoveMouth();
		}
		elapsed_time = curr_time;
		glutPostRedisplay();
	}
}
