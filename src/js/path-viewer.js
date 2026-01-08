/******************************************************************************\

                  This file is part of the Buildbotics firmware.

         Copyright (c) 2015 - 2026, Buildbotics LLC, All rights reserved.

          This Source describes Open Hardware and is licensed under the
                                  CERN-OHL-S v2.

          You may redistribute and modify this Source and make products
     using it under the terms of the CERN-OHL-S v2 (https:/cern.ch/cern-ohl).
            This Source is distributed WITHOUT ANY EXPRESS OR IMPLIED
     WARRANTY, INCLUDING OF MERCHANTABILITY, SATISFACTORY QUALITY AND FITNESS
      FOR A PARTICULAR PURPOSE. Please see the CERN-OHL-S v2 for applicable
                                   conditions.

                 Source location: https://github.com/buildbotics

       As per CERN-OHL-S v2 section 4, should You produce hardware based on
     these sources, You must maintain the Source Location clearly visible on
     the external case of the CNC Controller or other product you make using
                                   this Source.

                 For more information, email info@buildbotics.com

\******************************************************************************/


let OrbitControls = require('./orbit')
let cookie = require('./cookie')
let font   = require('./helvetiker_regular.typeface.json')


function get(obj, name, defaultValue) {
  return typeof obj[name] == 'undefined' ? defaultValue : obj[name]
}


function sizeOf(obj) {
  obj.geometry.computeBoundingBox()
  return obj.geometry.boundingBox.getSize(new THREE.Vector3())
}


let surfaceModes = ['cut', 'wire', 'solid', 'off']


module.exports = {
  template: '#path-viewer-template',
  props: ['toolpath', 'state', 'config'],


  data() {
    return {
      enabled: false,
      dirty: true,
      snapView: cookie.get('snap-view', 'angled'),
      surfaceMode: 'cut',
      axes: {},
      show: {
        path: cookie.get_bool('show-path', true),
        tool: cookie.get_bool('show-tool', true),
        bbox: cookie.get_bool('show-bbox', true),
        axes: cookie.get_bool('show-axes', true),
        grid: cookie.get_bool('show-grid', true),
        dims: cookie.get_bool('show-dims', true),
        intensity: cookie.get_bool('show-intensity', false)
      }
    }
  },


  computed: {
    target() {return $(this.$el).find('.path-viewer-content')[0]},


    metric() {
      return this.config.settings.units.toLowerCase() == 'metric'
    },


    envelope() {
      if (!this.axes.homed || !this.enabled) return undefined

      let min = new THREE.Vector3()
      let max = new THREE.Vector3()

      for (let axis of 'xyz') {
        min[axis] = this[axis].min - this[axis].off
        max[axis] = this[axis].max - this[axis].off
      }

      return new THREE.Box3(min, max)
    }
  },


  watch: {
    envelope() {this.redraw()},
    metric() {this.redraw()},
    surfaceMode(mode) {this.update_surface_mode(mode)},

    x() {this.axis_changed()},
    y() {this.axis_changed()},
    z() {this.axis_changed()}
  },


  ready() {this.graphics()},


  methods: {
    // From axis-vars
    get_bounds() {return this.toolpath.bounds},


    setShow(name, show) {
      this.show[name] = show
      cookie.set_bool('show-' + name, show)

      if (name == 'path')      this.pathView.visible = show
      if (name == 'tool')      this.toolView.visible = show
      if (name == 'axes')      this.axesView.visible = show
      if (name == 'grid')      this.gridView.visible = show
      if (name == 'dims')      this.dimsView.visible = show
      if (name == 'intensity') this.redraw()

      this.render_frame()
    },


    getShow(name) {return this.show[name]},
    toggle(name) {this.setShow(name, !this.getShow(name))},


    clear() {
      this.scene = new THREE.Scene()
      if (this.renderer != undefined) this.render_frame()
    },


    redraw() {
      if (!this.enabled) return
      this.scene = new THREE.Scene()
      this.draw(this.scene)
    },


    update() {
      if (!this.enabled || !this.toolpath.positions) return
      this.dirty = true
      this.positions = this.toolpath.positions
      this.speeds = this.toolpath.speeds
      this.redraw()
      this.snap(this.snapView)
      this.update_view()
    },


    update_surface_mode(mode) {
      if (!this.enabled) return

      if (typeof this.surfaceMaterial != 'undefined') {
        this.surfaceMaterial.wireframe = mode == 'wire'
        this.surfaceMaterial.needsUpdate = true
      }

      this.set_visible(this.surfaceMesh, mode == 'cut' || mode == 'wire')
      this.set_visible(this.workpieceMesh, mode == 'solid')
    },


    load_surface(surface) {
      if (typeof surface == 'undefined') {
        this.vertices = undefined
        this.normals = undefined
        return
      }

      this.vertices = surface.vertices

      // Expand normals
      this.normals = []
      for (let i = 0; i < surface.normals.length / 3; i++)
        for (let j = 0; j < 3; j++)
          for (let k = 0; k < 3; k++)
            this.normals.push(surface.normals[i * 3 + k])
    },


    set_visible(target, visible) {
      if (typeof target != 'undefined') target.visible = visible
      this.dirty = true
    },


    get_dims() {
      let t = $(this.target)
      let width = t.innerWidth()
      let height = t.innerHeight()
      return {width: width, height: height}
    },


    update_view() {
      if (!this.enabled) return
      let dims = this.get_dims()

      this.camera.aspect = dims.width / dims.height
      this.camera.updateProjectionMatrix()
      this.renderer.setSize(dims.width, dims.height)
      this.dirty = true
    },


    update_tool(tool) {
      if (!this.enabled) return
      if (tool == undefined) tool = this.toolView
      if (tool == undefined) return
      tool.position.x = this.x.pos
      tool.position.y = this.y.pos
      tool.position.z = this.z.pos
    },


    axis_changed() {
      if (!this.enabled) return
      this.update_tool()
      this.dirty = true
    },


    graphics() {
      try {
        // Renderer
        this.renderer = new THREE.WebGLRenderer({
          antialias: true,
          alpha: true
        })
        this.renderer.setPixelRatio(window.devicePixelRatio)
        this.renderer.setClearColor(0, 0)
        this.target.appendChild(this.renderer.domElement)

      } catch (e) {
        console.log('WebGL not supported: ', e)
        return
      }
      this.enabled = true

      // Camera
      this.camera = new THREE.PerspectiveCamera(45, 1, 1, 10000)
      this.camera.up.set(0, 0, 1)

      // Lighting
      this.ambient = new THREE.AmbientLight(0xffffff, 0.5)

      let keyLight = new THREE.DirectionalLight
      (new THREE.Color('hsl(30, 100%, 75%)'), 0.75)
      keyLight.position.set(-100, 0, 100)

      let fillLight = new THREE.DirectionalLight
      (new THREE.Color('hsl(240, 100%, 75%)'), 0.25)
      fillLight.position.set(100, 0, 100)

      let backLight = new THREE.DirectionalLight(0xffffff, 0.5)
      backLight.position.set(100, 0, -100).normalize()

      this.lights = new THREE.Group()
      this.lights.add(keyLight)
      this.lights.add(fillLight)
      this.lights.add(backLight)

      // Surface material
      this.surfaceMaterial = this.create_surface_material()

      // Controls
      this.controls = new OrbitControls(this.camera, this.renderer.domElement)
      this.controls.enableDamping = true
      this.controls.dampingFactor = 0.2
      this.controls.rotateSpeed = 0.5

      // Move lights with scene
      this.controls.addEventListener('change', scope => {
        return () => {
          keyLight.position.copy(scope.camera.position)
          fillLight.position.copy(scope.camera.position)
          backLight.position.copy(scope.camera.position)
          keyLight.lookAt(scope.controls.target)
          fillLight.lookAt(scope.controls.target)
          backLight.lookAt(scope.controls.target)
          scope.dirty = true
        }
      })

      // Events
      window.addEventListener('resize', this.update_view, false)

      // Start it
      this.render()
    },


    create_surface_material() {
      return new THREE.MeshPhongMaterial({
        specular: 0x111111,
        shininess: 10,
        side: THREE.FrontSide,
        color: 0x0c2d53
      })
    },


    draw_workpiece(scene, material) {
      if (typeof this.workpiece == 'undefined') return undefined

      let min = this.workpiece.min
      let max = this.workpiece.max

      min = new THREE.Vector3(min[0], min[1], min[2])
      max = new THREE.Vector3(max[0], max[1], max[2])
      let dims = max.clone().sub(min)

      let geometry = new THREE.BoxGeometry(dims.x, dims.y, dims.z)
      let mesh = new THREE.Mesh(geometry, material)

      let offset = dims.clone()
      offset.divideScalar(2)
      offset.add(min)

      mesh.position.add(offset)

      geometry.computeBoundingBox()

      scene.add(mesh)

      return mesh
    },


    draw_surface(scene, material) {
      if (typeof this.vertices == 'undefined') return

      let geometry = new THREE.BufferGeometry()

      geometry.setAttribute
      ('position', new THREE.Float32BufferAttribute(this.vertices, 3))
      geometry.setAttribute
      ('normal', new THREE.Float32BufferAttribute(this.normals, 3))

      geometry.computeBoundingSphere()
      geometry.computeBoundingBox()

      return new THREE.Mesh(geometry, material)
    },


    draw_tool(scene, bbox) {
      if (bbox.isEmpty()) return new THREE.Group()

      // Tool size is relative to bounds
      let size = bbox.getSize(new THREE.Vector3())
      let length = (size.x + size.y + size.z) / 24

      if (length < 1) length = 1

      let material = new THREE.MeshPhongMaterial({
        transparent: true,
        opacity: 0.75,
        specular: 0x161616,
        shininess: 10,
        color: 0xffa500 // Orange
      })

      let geometry = new THREE.CylinderGeometry(length / 2, 0, length, 128)
      geometry.translate(0, length / 2, 0)
      geometry.rotateX(0.5 * Math.PI)

      let mesh = new THREE.Mesh(geometry, material)
      this.update_tool(mesh)
      mesh.visible = this.show.tool
      scene.add(mesh)
      return mesh
    },


    draw_axis(axis, up, length, radius) {
      let color

      if (axis == 0)      color = 0xff0000 // Red
      else if (axis == 1) color = 0x00ff00 // Green
      else if (axis == 2) color = 0x0000ff // Blue

      let group = new THREE.Group()
      let material = new THREE.MeshPhongMaterial({
          specular: 0x161616, shininess: 10, color: color
      })
      let geometry = new THREE.CylinderGeometry(radius, radius, length, 128)
      geometry.translate(0, -length / 2, 0)
      group.add(new THREE.Mesh(geometry, material))

      geometry = new THREE.CylinderGeometry(1.5 * radius, 0, 2 * radius, 128)
      geometry.translate(0, -length - radius, 0)
      group.add(new THREE.Mesh(geometry, material))

      if (axis == 0)      group.rotateZ((up ? 0.5 : 1.5) * Math.PI)
      else if (axis == 1) group.rotateX((up ? 0   : 1  ) * Math.PI)
      else if (axis == 2) group.rotateX((up ? 1.5 : 0.5) * Math.PI)

      return group
    },


    draw_axes(scene, bbox) {
      let size = bbox.getSize(new THREE.Vector3())
      let length = (size.x + size.y + size.z) / 3
      length /= 10

      if (length < 1) length = 1

      let radius = length / 20

      let group = new THREE.Group()

      for (let axis = 0; axis < 3; axis++)
        for (let up = 0; up < 2; up++)
          group.add(this.draw_axis(axis, up, length, radius))

      group.visible = this.show.axes
      scene.add(group)

      return group
    },


    draw_grid(scene, bbox) {
      // Grid size is relative to bounds
      let size = bbox.getSize(new THREE.Vector3())
      size = Math.max(size.x, size.y) * 16
      let step = this.metric ? 10 : 25.4
      let divs = Math.ceil(size / step)
      size = divs * step

      let material = new THREE.MeshPhongMaterial({
        shininess: 0,
        specular: 0,
        color: 0,
        opacity: 0.2,
        transparent: true
      })

      let grid = new THREE.GridHelper(size, divs)
      grid.material = material
      grid.rotation.x = Math.PI / 2

      grid.visible = this.show.grid
      scene.add(grid)

      return grid
    },


    draw_text(text, size, color) {
      let geometry = new THREE.TextGeometry(text, {
        font: new THREE.Font(font),
        size: size,
        height: 0.001,
        curveSegments: 12,
        bevelEnabled: false
      })

      let material = new THREE.MeshBasicMaterial({color: color})

      return new THREE.Mesh(geometry, material)
    },


    format_dim(dim) {
      if (!this.metric) dim /= 25.4
      return dim.toFixed(1) + (this.metric ? ' mm' : ' in')
    },


    draw_box_dims(bounds, color) {
      let group = new THREE.Group()

      let dims = bounds.getSize(new THREE.Vector3())
      let size = Math.max(dims.x, dims.y, dims.z) / 40

      let xDim = this.draw_text(this.format_dim(dims.x), size, color)
      xDim.position.x = bounds.min.x + (dims.x - sizeOf(xDim).x) / 2
      xDim.position.y = bounds.max.y + size
      xDim.position.z = bounds.max.z
      group.add(xDim)

      let yDim = this.draw_text(this.format_dim(dims.y), size, color)
      yDim.position.x = bounds.max.x + size
      yDim.position.y = bounds.min.y + (dims.y + sizeOf(yDim).x) / 2
      yDim.position.z = bounds.max.z
      yDim.rotateZ(-Math.PI / 2)
      group.add(yDim)

      let zDim = this.draw_text(this.format_dim(dims.z), size, color)
      zDim.position.x = bounds.max.x + size
      zDim.position.y = bounds.max.y
      zDim.position.z = bounds.min.z + (dims.z - sizeOf(zDim).y) / 2
      zDim.rotateX(Math.PI / 2)
      group.add(zDim)

      let material = new THREE.LineBasicMaterial({
        linewidth: 2,
        color: color,
        opacity: 0.4,
        transparent: true
      })

      // Prevent any zero dimensions
      bounds = bounds.clone()
      if (bounds.min.x == bounds.max.x) bounds.max.x += 0.0000001
      if (bounds.min.y == bounds.max.y) bounds.max.y += 0.0000001
      if (bounds.min.z == bounds.max.z) bounds.max.z += 0.0000001

      let box = new THREE.Box3Helper(bounds)
      box.material = material
      group.add(box)

      return group
    },


    draw_dims(scene, bbox) {
      let group = new THREE.Group()
      group.visible = this.show.dims
      scene.add(group)

      if (!bbox.isEmpty()) {
        // Bounds
        group.add(this.draw_box_dims(bbox, 0x0c2d53))

        // Envelope
        if (this.envelope)
          group.add(this.draw_box_dims(this.envelope, 0x00f7ff))
      }

      return group
    },


    get_color(speed) {
      if (isNaN(speed)) return [255, 0, 0] // Rapid

      let intensity = speed / this.toolpath.maxSpeed
      if (typeof speed == 'undefined' || !this.show.intensity) intensity = 1
      return [0, 255 * intensity, 127 * (1 - intensity)]
    },


    draw_path(scene) {
      if (!this.positions || this.positions.length < 6) return new THREE.Group()

      let geometry = new THREE.BufferGeometry()
      let material =
          new THREE.LineBasicMaterial({
            vertexColors: THREE.VertexColors,
            linewidth: 1.5
          })

      let positions = new THREE.Float32BufferAttribute(this.positions, 3)
      geometry.setAttribute('position', positions)

      let colors = []
      for (let i = 0; i < this.speeds.length; i++) {
        let color = this.get_color(this.speeds[i])
        Array.prototype.push.apply(colors, color)
      }

      colors = new THREE.Uint8BufferAttribute(colors, 3, true)
      geometry.setAttribute('color', colors)

      geometry.computeBoundingSphere()
      geometry.computeBoundingBox()

      let line = new THREE.Line(geometry, material)

      line.visible = this.show.path
      scene.add(line)

      return line
    },


    draw(scene) {
      // Lights
      scene.add(this.ambient)
      scene.add(this.lights)

      // Model
      this.pathView      = this.draw_path(scene)
      this.surfaceMesh   = this.draw_surface(scene, this.surfaceMaterial)
      this.workpieceMesh = this.draw_workpiece(scene, this.surfaceMaterial)
      this.update_surface_mode(this.surfaceMode)

      // Compute bounding box
      let bbox = this.get_model_bounds()
      let realBBox = this.get_model_bounds(true)

      // Tool, axes & bounds
      this.toolView = this.draw_tool(scene, realBBox)
      this.axesView = this.draw_axes(scene, bbox)
      this.gridView = this.draw_grid(scene, bbox)
      this.dimsView = this.draw_dims(scene, realBBox)
    },


    render_frame() {this.renderer.render(this.scene, this.camera)},


    render() {
      window.requestAnimationFrame(this.render)
      if (this.scene == undefined) return

      if (this.controls.update() || this.dirty) {
        this.dirty = false
        this.render_frame()
      }
    },


    get_model_bounds(real) {
      let bbox

      function add(o) {
        if (o == undefined) return

        let oBBox = new THREE.Box3()
        oBBox.setFromObject(o)
        if (bbox == undefined) bbox = oBBox
        else bbox.union(oBBox)
      }

      add(this.pathView)
      add(this.surfaceMesh)
      add(this.workpieceMesh)

      if (bbox.isEmpty() && !real)
        bbox = new THREE.Box3(new THREE.Vector3(-10, -10, -10),
                              new THREE.Vector3(10, 10, 10))

      return bbox
    },


    snap(view) {
      if (view != this.snapView) {
        this.snapView = view
        cookie.set('snap-view', view)
      }

      let bbox = this.get_model_bounds()
      let corners = [
        new THREE.Vector3(bbox.min.x, bbox.min.y, bbox.min.z),
        new THREE.Vector3(bbox.min.x, bbox.min.y, bbox.max.z),
        new THREE.Vector3(bbox.min.x, bbox.max.y, bbox.min.z),
        new THREE.Vector3(bbox.min.x, bbox.max.y, bbox.max.z),
        new THREE.Vector3(bbox.max.x, bbox.min.y, bbox.min.z),
        new THREE.Vector3(bbox.max.x, bbox.min.y, bbox.max.z),
        new THREE.Vector3(bbox.max.x, bbox.max.y, bbox.min.z),
        new THREE.Vector3(bbox.max.x, bbox.max.y, bbox.max.z),
      ]

      this.controls.reset()
      bbox.getCenter(this.controls.target)
      this.update_view()

      // Compute new camera position
      let center = bbox.getCenter(new THREE.Vector3())
      let offset = new THREE.Vector3()

      if (view == 'angled') {offset.y -= 1; offset.z += 1}
      if (view == 'front')  offset.y -= 1
      if (view == 'back')   offset.y += 1
      if (view == 'left')   {offset.x -= 1; offset.z += 0.0001}
      if (view == 'right')  {offset.x += 1; offset.z += 0.0001}
      if (view == 'top')    offset.z += 1
      if (view == 'bottom') offset.z -= 1
      offset.normalize()

      // Initial camera position
      let position = center.clone().add(offset)
      this.camera.position.copy(position)
      this.camera.lookAt(center) // Get correct camera orientation

      let theta = this.camera.fov / 180 * Math.PI // View angle
      let cameraLine = new THREE.Line3(center, position)
      let cameraUp = new THREE.Vector3().copy(this.camera.up)
          .applyQuaternion(this.camera.quaternion)
      let cameraLeft =
          new THREE.Vector3().copy(offset).cross(cameraUp).normalize()

      let dist = this.camera.near // Min camera dist

      for (let i = 0; i < corners.length; i++) {
        // Project on to camera line
        let p1 = cameraLine
            .closestPointToPoint(corners[i], false, new THREE.Vector3())

        // Compute distance from projection to center
        let d = p1.distanceTo(center)
        if (cameraLine.closestPointToPointParameter(p1, false) < 0) d = -d

        // Compute up line
        let up =
            new THREE.Line3(p1, new THREE.Vector3().copy(p1).add(cameraUp))

        // Project on to up line
        let p2 = up.closestPointToPoint(corners[i], false, new THREE.Vector3())

        // Compute length
        let l = p1.distanceTo(p2)

        // Update min camera distance
        dist = Math.max(dist, d + l / Math.tan(theta / 2) / this.camera.aspect)

        // Compute left line
        let left =
            new THREE.Line3(p1, new THREE.Vector3().copy(p1).add(cameraLeft))

        // Project on to left line
        let p3 =
            left.closestPointToPoint(corners[i], false, new THREE.Vector3())

        // Compute length
        l = p1.distanceTo(p3)

        // Update min camera distance
        dist = Math.max(dist, d + l / Math.tan(theta / 2))
      }

      this.camera.position.copy(offset.multiplyScalar(dist * 1.2).add(center))
    }
  },


  mixins: [require('./axis-vars')]
}
