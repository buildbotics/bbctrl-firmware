/******************************************************************************\

                    Copyright 2018. Buildbotics LLC
                              All Rights Reserved.

                  For information regarding this software email:
                                 Joseph Coffland
                              joseph@buildbotics.com

        This software is free software: you clan redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
        as published by the Free Software Foundation, either version 2.1 of
        the License, or (at your option) any later version.

        This software is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
        Lesser General Public License for more details.

        You should have received a copy of the GNU Lesser General Public
        License along with the C! library.  If not, see
        <http://www.gnu.org/licenses/>.

\******************************************************************************/

'use strict'

var orbit = require('./orbit');
var cookie = require('./cookie')('bbctrl-');


function get(obj, name, defaultValue) {
  return typeof obj[name] == 'undefined' ? defaultValue : obj[name];
}


function set_visible(target, visible) {
  if (typeof target != 'undefined') target.visible = visible;
}


var surfaceModes = ['cut', 'wire', 'solid', 'off'];


module.exports = {
  template: '#path-viewer-template',
  props: ['toolpath', 'progress'],


  data: function () {
    return {
      enabled: false,
      loading: false,
      snapView: cookie.get('snap-view', 'isometric'),
      small: cookie.get_bool('small-path-view', true),
      surfaceMode: 'cut',
      showPath: cookie.get_bool('show-path', true),
      showTool: cookie.get_bool('show-tool', true),
      showBBox: cookie.get_bool('show-bbox', true),
      showAxes: cookie.get_bool('show-axes', true),
      showIntensity: cookie.get_bool('show-intensity', false)
    }
  },


  computed: {
    hasPath: function () {return typeof this.toolpath.path != 'undefined'},
    target: function () {return $(this.$el).find('.path-viewer-content')[0]}
  },


  watch: {
    toolpath: function () {Vue.nextTick(this.update)},
    surfaceMode: function (mode) {this.update_surface_mode(mode)},


    small: function (enable) {
      cookie.set_bool('small-path-view', enable);
      Vue.nextTick(this.update_view)
    },


    showPath: function (enable) {
      cookie.set_bool('show-path', enable);
      set_visible(this.pathView, enable)
    },


    showTool: function (enable) {
      cookie.set_bool('show-tool', enable);
      set_visible(this.toolView, enable)
    },


    showAxes: function (enable) {
      cookie.set_bool('show-axes', enable);
      set_visible(this.axesView, enable)
    },


    showIntensity: function (enable) {
      cookie.set_bool('show-intensity', enable);
      Vue.nextTick(this.update)
    },


    showBBox: function (enable) {
      cookie.set_bool('show-bbox', enable);
      set_visible(this.bboxView, enable);
      set_visible(this.envelopeView, enable);
    },


    x: function () {this.axis_changed()},
    y: function () {this.axis_changed()},
    z: function () {this.axis_changed()}
  },


  ready: function () {
    this.graphics();
    if (typeof this.toolpath.path != 'undefined') Vue.nextTick(this.update);
  },


  methods: {
    update: function () {
      if (!this.enabled) return;

      // Reset message
      this.loading = !this.hasPath;

      // Update scene
      this.scene = new THREE.Scene();
      if (this.hasPath) {
        this.draw(this.scene);
        this.snap(this.snapView);
      }

      this.update_view();
    },


    update_surface_mode: function (mode) {
      if (!this.enabled) return;

      if (typeof this.surfaceMaterial != 'undefined') {
        this.surfaceMaterial.wireframe = mode == 'wire';
        this.surfaceMaterial.needsUpdate = true;
      }

      set_visible(this.surfaceMesh, mode == 'cut' || mode == 'wire');
      set_visible(this.workpieceMesh, mode == 'solid');
    },


    load_surface: function (surface) {
      if (typeof surface == 'undefined') {
        this.vertices = undefined;
        this.normals = undefined;
        return;
      }

      this.vertices = surface.vertices;

      // Expand normals
      this.normals = [];
      for (var i = 0; i < surface.normals.length / 3; i++)
        for (var j = 0; j < 3; j++)
          for (var k = 0; k < 3; k++)
            this.normals.push(surface.normals[i * 3 + k]);
    },


    get_dims: function () {
      var t = $(this.target);
      var width = t.innerWidth();
      var height = t.innerHeight();
      return {width: width, height: height};
    },


    update_view: function () {
      if (!this.enabled) return;
      var dims = this.get_dims();

      this.camera.aspect = dims.width / dims.height;
      this.camera.updateProjectionMatrix();
      this.renderer.setSize(dims.width, dims.height);
    },


    update_tool: function (tool) {
      if (!this.enabled) return;
      if (typeof tool == 'undefined') tool = this.toolView;
      if (typeof tool == 'undefined') return;
      tool.position.x = this.x.pos;
      tool.position.y = this.y.pos;
      tool.position.z = this.z.pos;
    },


    update_envelope: function (envelope) {
      if (!this.enabled || !this.axes.homed) return;
      if (typeof envelope == 'undefined') envelope = this.envelopeView;
      if (typeof envelope == 'undefined') return;

      var min = new THREE.Vector3();
      var max = new THREE.Vector3();

      for (var axis of 'xyz') {
        min[axis] = this[axis].min + this[axis].off;
        max[axis] = this[axis].max + this[axis].off;
      }

      var bounds = new THREE.Box3(min, max);
      if (bounds.isEmpty()) envelope.geometry = this.create_empty_geom();
      else envelope.geometry = this.create_bbox_geom(bounds);
    },


    axis_changed: function () {
      this.update_tool();
      this.update_envelope();
    },


    graphics: function () {
      try {
        // Renderer
        this.renderer = new THREE.WebGLRenderer({antialias: true, alpha: true});
        this.renderer.setPixelRatio(window.devicePixelRatio);
        this.renderer.setClearColor(0, 0);

        this.target.appendChild(this.renderer.domElement);

      } catch (e) {
        console.log('WebGL not supported: ', e);
        return;
      }
      this.enabled = true;

      // Camera
      this.camera = new THREE.PerspectiveCamera(45, 4 / 3, 1, 1000);

      // Lighting
      this.ambient = new THREE.AmbientLight(0xffffff, 0.5);

      var keyLight = new THREE.DirectionalLight
      (new THREE.Color('hsl(30, 100%, 75%)'), 0.75);
      keyLight.position.set(-100, 0, 100);

      var fillLight = new THREE.DirectionalLight
      (new THREE.Color('hsl(240, 100%, 75%)'), 0.25);
      fillLight.position.set(100, 0, 100);

      var backLight = new THREE.DirectionalLight(0xffffff, 0.5);
      backLight.position.set(100, 0, -100).normalize();

      this.lights = new THREE.Group();
      this.lights.add(keyLight);
      this.lights.add(fillLight);
      this.lights.add(backLight);

      // Surface material
      this.surfaceMaterial = this.create_surface_material();

      // Controls
      this.controls = new orbit(this.camera, this.renderer.domElement);
      this.controls.enableDamping = true;
      this.controls.dampingFactor = 0.2;
      this.controls.rotateSpeed = 0.25;
      this.controls.enableZoom = true;

      // Move lights with scene
      this.controls.addEventListener('change', function (scope) {
        return function () {
          keyLight.position.copy(scope.camera.position);
          fillLight.position.copy(scope.camera.position);
          backLight.position.copy(scope.camera.position);
          keyLight.lookAt(scope.controls.target);
          fillLight.lookAt(scope.controls.target);
          backLight.lookAt(scope.controls.target);
        }
      }(this))

      // Events
      window.addEventListener('resize', this.update_view, false);

      // Start it
      this.render();
    },


    create_surface_material: function () {
      return new THREE.MeshPhongMaterial({
        specular: 0x111111,
        shininess: 10,
        side: THREE.FrontSide,
        color: 0x0c2d53
      });
    },


    draw_workpiece: function (scene, material) {
      if (typeof this.workpiece == 'undefined') return;

      var min = this.workpiece.min;
      var max = this.workpiece.max;

      min = new THREE.Vector3(min[0], min[1], min[2]);
      max = new THREE.Vector3(max[0], max[1], max[2]);
      var dims = max.clone().sub(min);

      var geometry = new THREE.BoxGeometry(dims.x, dims.y, dims.z)
      var mesh = new THREE.Mesh(geometry, material);

      var offset = dims.clone();
      offset.divideScalar(2);
      offset.add(min);

      mesh.position.add(offset);

      geometry.computeBoundingBox();

      scene.add(mesh);

      return mesh;
    },


    draw_surface: function (scene, material) {
      if (typeof this.vertices == 'undefined') return;

      var geometry = new THREE.BufferGeometry();

      geometry.addAttribute
      ('position', new THREE.Float32BufferAttribute(this.vertices, 3));
      geometry.addAttribute
      ('normal', new THREE.Float32BufferAttribute(this.normals, 3));

      geometry.computeBoundingSphere();
      geometry.computeBoundingBox();

      return new THREE.Mesh(geometry, material);
    },


    draw_tool: function (scene, bbox) {
      // Tool size is relative to bounds
      var size = bbox.getSize(new THREE.Vector3());
      var length = (size.x + size.y + size.z) / 24;

      var material = new THREE.MeshPhongMaterial({
        transparent: true,
        opacity: 0.75,
        specular: 0x161616,
        shininess: 10,
        color: 0xffa500 // Orange
      });

      var geometry = new THREE.CylinderGeometry(length / 2, 0, length, 128);
      geometry.translate(0, length / 2, 0);
      geometry.rotateX(0.5 * Math.PI);

      var mesh = new THREE.Mesh(geometry, material);
      this.update_tool(mesh);
      mesh.visible = this.showTool;
      scene.add(mesh);
      return mesh;
    },


    draw_axis: function (axis, up, length, radius) {
      var color;

      if (axis == 0)      color = 0xff0000; // Red
      else if (axis == 1) color = 0x00ff00; // Green
      else if (axis == 2) color = 0x0000ff; // Blue

      var group = new THREE.Group();
      var material = new THREE.MeshPhongMaterial({
          specular: 0x161616, shininess: 10, color: color
      });
      var geometry = new THREE.CylinderGeometry(radius, radius, length, 128);
      geometry.translate(0, -length / 2, 0);
      group.add(new THREE.Mesh(geometry, material));

      geometry = new THREE.CylinderGeometry(1.5 * radius, 0, 2 * radius, 128);
      geometry.translate(0, -length - radius, 0);
      group.add(new THREE.Mesh(geometry, material));

      if (axis == 0)      group.rotateZ((up ? 0.5 : 1.5) * Math.PI);
      else if (axis == 1) group.rotateX((up ? 0   : 1  ) * Math.PI);
      else if (axis == 2) group.rotateX((up ? 1.5 : 0.5) * Math.PI);

      return group;
    },


    draw_axes: function (scene, bbox) {
      var size = bbox.getSize(new THREE.Vector3());
      var length = (size.x + size.y + size.z) / 3;
      length /= 10;
      var radius = length / 20;

      var group = new THREE.Group();

      for (var axis = 0; axis < 3; axis++)
        for (var up = 0; up < 2; up++)
          group.add(this.draw_axis(axis, up, length, radius));

      group.visible = this.showAxes;
      scene.add(group);

      return group;
    },


    get_color: function (rapid, speed) {
      if (rapid) return [1, 0, 0];

      var intensity = speed / this.toolpath.maxSpeed;
      if (typeof speed == 'undefined' || !this.showIntensity) intensity = 1;
      return [0, intensity, 0.5 * (1 - intensity)];
    },


    draw_path: function (scene) {
      var s = undefined;
      var x = this.x.pos;
      var y = this.y.pos;
      var z = this.z.pos;
      var color = undefined;

      var positions = [];
      var colors = [];

      for (var i = 0; i < this.toolpath.path.length; i++) {
        var step = this.toolpath.path[i];

        s = get(step, 's', s);
        var newColor = this.get_color(step.rapid, s);

        // Handle color change
        if (!i || newColor != color) {
          color = newColor;
          positions.push(x, y, z);
          colors.push.apply(colors, color);
        }

        // Draw to move target
        x = get(step, 'x', x);
        y = get(step, 'y', y);
        z = get(step, 'z', z);

        positions.push(x, y, z);
        colors.push.apply(colors, color);
      }

      var geometry = new THREE.BufferGeometry();
      var material =
          new THREE.LineBasicMaterial({
            vertexColors: THREE.VertexColors,
            linewidth: 1.5
          });

      geometry.addAttribute('position',
                            new THREE.Float32BufferAttribute(positions, 3));
      geometry.addAttribute('color',
                            new THREE.Float32BufferAttribute(colors, 3));

      geometry.computeBoundingSphere();
      geometry.computeBoundingBox();

      var line = new THREE.Line(geometry, material);

      line.visible = this.showPath;
      scene.add(line);

      return line;
    },


    create_empty_geom: function () {
      var geometry = new THREE.BufferGeometry();
      geometry.addAttribute('position',
                            new THREE.Float32BufferAttribute([], 3));
      return geometry;
    },


    create_bbox_geom: function (bbox) {
      var vertices = [];

      if (!bbox.isEmpty()) {
        // Top
        vertices.push(bbox.min.x, bbox.min.y, bbox.min.z);
        vertices.push(bbox.max.x, bbox.min.y, bbox.min.z);
        vertices.push(bbox.max.x, bbox.min.y, bbox.min.z);
        vertices.push(bbox.max.x, bbox.min.y, bbox.max.z);
        vertices.push(bbox.max.x, bbox.min.y, bbox.max.z);
        vertices.push(bbox.min.x, bbox.min.y, bbox.max.z);
        vertices.push(bbox.min.x, bbox.min.y, bbox.max.z);
        vertices.push(bbox.min.x, bbox.min.y, bbox.min.z);

        // Bottom
        vertices.push(bbox.min.x, bbox.max.y, bbox.min.z);
        vertices.push(bbox.max.x, bbox.max.y, bbox.min.z);
        vertices.push(bbox.max.x, bbox.max.y, bbox.min.z);
        vertices.push(bbox.max.x, bbox.max.y, bbox.max.z);
        vertices.push(bbox.max.x, bbox.max.y, bbox.max.z);
        vertices.push(bbox.min.x, bbox.max.y, bbox.max.z);
        vertices.push(bbox.min.x, bbox.max.y, bbox.max.z);
        vertices.push(bbox.min.x, bbox.max.y, bbox.min.z);

        // Sides
        vertices.push(bbox.min.x, bbox.min.y, bbox.min.z);
        vertices.push(bbox.min.x, bbox.max.y, bbox.min.z);
        vertices.push(bbox.max.x, bbox.min.y, bbox.min.z);
        vertices.push(bbox.max.x, bbox.max.y, bbox.min.z);
        vertices.push(bbox.max.x, bbox.min.y, bbox.max.z);
        vertices.push(bbox.max.x, bbox.max.y, bbox.max.z);
        vertices.push(bbox.min.x, bbox.min.y, bbox.max.z);
        vertices.push(bbox.min.x, bbox.max.y, bbox.max.z);
      }

      var geometry = new THREE.BufferGeometry();

      geometry.addAttribute('position',
                            new THREE.Float32BufferAttribute(vertices, 3));

      return geometry;
    },


    draw_bbox: function (scene, bbox) {
      var geometry = this.create_bbox_geom(bbox);
      var material = new THREE.LineBasicMaterial({color: 0xffffff});
      var line = new THREE.LineSegments(geometry, material);

      line.visible = this.showBBox;

      scene.add(line);

      return line;
    },


    draw_envelope: function (scene) {
      var geometry = this.create_empty_geom();
      var material = new THREE.LineBasicMaterial({color: 0x00f7ff});
      var line = new THREE.LineSegments(geometry, material);

      line.visible = this.showBBox;

      scene.add(line);
      this.update_envelope(line);

      return line;
    },


    draw: function (scene) {
      // Lights
      scene.add(this.ambient);
      scene.add(this.lights);

      // Model
      this.pathView = this.draw_path(scene);
      this.surfaceMesh = this.draw_surface(scene, this.surfaceMaterial);
      this.workpieceMesh = this.draw_workpiece(scene, this.surfaceMaterial);
      this.update_surface_mode(this.surfaceMode);

      // Compute bounding box
      var bbox = this.get_model_bounds();

      // Tool, axes & bounds
      this.toolView = this.draw_tool(scene, bbox);
      this.axesView = this.draw_axes(scene, bbox);
      this.bboxView = this.draw_bbox(scene, bbox);
      this.envelopeView = this.draw_envelope(scene);
    },


    render: function () {
      window.requestAnimationFrame(this.render);
      if (typeof this.scene == 'undefined') return;
      this.controls.update();
      this.renderer.render(this.scene, this.camera);
    },


    get_model_bounds: function () {
      var bbox = new THREE.Box3();

      function add(o) {
        if (typeof o != 'undefined') bbox.union(o.geometry.boundingBox);
      }

      add(this.pathView);
      add(this.surfaceMesh);
      add(this.workpieceMesh);

      return bbox;
    },


    snap: function (view) {
      if (view != this.snapView) {
        this.snapView = view;
        cookie.set('snap-view', view);
      }

      var bbox = this.get_model_bounds();
      this.controls.reset();
      bbox.getCenter(this.controls.target);
      this.update_view();

      // Compute new camera position
      var center = bbox.getCenter(new THREE.Vector3());
      var offset = new THREE.Vector3();

      if (view == 'isometric') {offset.y -= 1; offset.z += 1;}
      if (view == 'front')  offset.y -= 1;
      if (view == 'back')   offset.y += 1;
      if (view == 'left')   offset.x -= 1;
      if (view == 'right')  offset.x += 1;
      if (view == 'top')    offset.z += 1;
      if (view == 'bottom') offset.z -= 1;
      offset.normalize();

      // Initial camera position
      var position = new THREE.Vector3().copy(center).add(offset);
      this.camera.position.copy(position);
      this.camera.lookAt(center); // Get correct camera orientation

      var theta = this.camera.fov / 180 * Math.PI; // View angle
      var cameraLine = new THREE.Line3(center, position);
      var cameraUp = new THREE.Vector3().copy(this.camera.up)
          .applyQuaternion(this.camera.quaternion);
      var cameraLeft =
          new THREE.Vector3().copy(offset).cross(cameraUp).normalize();

      var corners = [
        new THREE.Vector3(bbox.min.x, bbox.min.y, bbox.min.z),
        new THREE.Vector3(bbox.min.x, bbox.min.y, bbox.max.z),
        new THREE.Vector3(bbox.min.x, bbox.max.y, bbox.min.z),
        new THREE.Vector3(bbox.min.x, bbox.max.y, bbox.max.z),
        new THREE.Vector3(bbox.max.x, bbox.min.y, bbox.min.z),
        new THREE.Vector3(bbox.max.x, bbox.min.y, bbox.max.z),
        new THREE.Vector3(bbox.max.x, bbox.max.y, bbox.min.z),
        new THREE.Vector3(bbox.max.x, bbox.max.y, bbox.max.z),
      ]

      var dist = this.camera.near; // Min camera dist

      for (var i = 0; i < corners.length; i++) {
        // Project on to camera line
        var p1 = cameraLine
            .closestPointToPoint(corners[i], false, new THREE.Vector3());

        // Compute distance from projection to center
        var d = p1.distanceTo(center);
        if (cameraLine.closestPointToPointParameter(p1, false) < 0) d = -d;

        // Compute up line
        var up =
            new THREE.Line3(p1, new THREE.Vector3().copy(p1).add(cameraUp));

        // Project on to up line
        var p2 = up.closestPointToPoint(corners[i], false, new THREE.Vector3());

        // Compute length
        var l = p1.distanceTo(p2);

        // Update min camera distance
        dist = Math.max(dist, d + l / Math.tan(theta / 2));

        // Compute left line
        var left =
            new THREE.Line3(p1, new THREE.Vector3().copy(p1).add(cameraLeft));

        // Project on to left line
        var p3 =
            left.closestPointToPoint(corners[i], false, new THREE.Vector3());

        // Compute length
        l = p1.distanceTo(p3);

        // Update min camera distance
        dist = Math.max(dist, d + l / Math.tan(theta / 2) / this.camera.aspect);
      }

      this.camera.position.copy(offset.multiplyScalar(dist * 1.2).add(center));
    }
  },


  mixins: [require('./axis-vars')]
}
