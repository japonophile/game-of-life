(ns game-of-life.core
  (:require [reagent.core :as r]
            [reagent.dom :as rd]))

(enable-console-print!)

; constants
(def width 100)
(def height 50)
(def d 10) ; cell size in pixels
(def step-time-ms 500)

(defn empty-world
  []
  (vec (repeat height (vec (repeat width 0)))))

(defn initial-state
  ([]
   (initial-state true))
  ([playing]
   {:world (empty-world) :playing? playing :steps 0 :current-step 0 :history []}))

(defonce app-state (r/atom (initial-state)))

; reset state at every reload
(reset! app-state (initial-state))
(swap! app-state update :world
  #(-> %
     (assoc-in [27 47] 1)
     (assoc-in [27 49] 1)
     (assoc-in [26 49] 1)
     (assoc-in [25 51] 1)
     (assoc-in [24 51] 1)
     (assoc-in [23 51] 1)
     (assoc-in [24 53] 1)
     (assoc-in [23 53] 1)
     (assoc-in [22 53] 1)
     (assoc-in [23 54] 1)))

(println "Conway's Game of Life")

(defn count-neighbors
  [world r c]
  (let [r1 (max 0 (dec r))
        r2 (min (inc r) (count world))
        c1 (max 0 (dec c))
        c2 (min (inc c) (count (get world 0)))]
    (- (reduce + (for [y (range r1 (inc r2))
                       x (range c1 (inc c2))]
                   (get-in world [y x])))
       (get-in world [r c]))))

(defn update-world
  [world]
  (vec (for [r (range height)]
         (vec (for [c (range width)]
                (let [n (count-neighbors world r c)]
                  (if (= 1 (get-in world [r c]))
                    (if (and (< 1 n) (< n 4)) 1 0)   ; 1
                    (if (= 3 n) 1 0))))))))          ; 0

(defn single-step-update
  [state]
  (let [steps (inc (:steps state))]
    (assoc state
           :world (update-world (:world state))
           :steps steps
           :current-step steps
           :history (conj (:history state) (:world state)))))

(defn update-state
  [state]
  (if (:playing? state)
    (single-step-update state)
    state))

(defn render-world
  []
  (let [world (if (= (:current-step @app-state) (:steps @app-state))
                (:world @app-state)
                (get (:history @app-state) (:current-step @app-state)))
        canvas (.getElementById js/document "world")
        ctx (.getContext canvas "2d")]
    (doall (for [r (range height)
                 c (range width)]
             (do
               (if (= 1 (get-in world [r c]))
                 (set! (.-fillStyle ctx) "rgb(0,0,0)")
                 (set! (.-fillStyle ctx) "rgb(255,255,255)"))
               (.beginPath ctx)
               (.rect ctx (* d c) (* d r) d d)
               (.fill ctx))))))

(defn toggle-cell
  [r c]
  (swap! app-state update-in [:world r c] #(- 1 %)))

(defn toggle-cell-action
  [e]
  (let [rect (-> e .-target .getBoundingClientRect)
        t (-> rect .-top)
        l (-> rect .-left)
        r (/ (- (-> e .-clientY) t) d)
        c (/ (- (-> e .-clientX) l) d)]
    (toggle-cell r c)))

(defn reset-action
  [_]
  (reset! app-state (initial-state false)))

(defn pause-action
  [_]
  (swap! app-state assoc :playing? false))

(defn play-action
  [_]
  (swap! app-state #(assoc % :current-step (:steps %) :playing? true)))

(defn step-action
  [_]
  (swap! app-state #(assoc % :current-step (:steps %)))
  (swap! app-state single-step-update))

(defn slider-action
  [e]
  (swap! app-state assoc :current-step (js/parseInt (-> e .-target .-value) 10)))

(defn world-component
  []
  (r/create-class
    {
     :component-did-update
     (fn [_]
       (render-world))
     :component-did-mount
     (fn [_]
       (render-world))
     :reagent-render
     (fn []
       (:world @app-state)
       [:div {:style {:width "100%"}}
        [:canvas {:id "world" :width (str (* d width)) :height (str (* d height))
                  :style {:margin "auto" :border "1px solid black"}
                  :on-click toggle-cell-action}]])}))

(defn app-component
  []
  [:div
   [:h1 "The Game of Life"]
   [world-component]
   [:div
    [:button.btn {:disabled (:playing? @app-state)
                  :on-click reset-action} [:i.fa.fa-trash]]
    [:span " "]
    [:button.btn {:disabled (not (:playing? @app-state))
                  :on-click pause-action} [:i.fa.fa-pause]]
    [:span " "]
    [:button.btn {:disabled (:playing? @app-state)
                  :on-click play-action} [:i.fa.fa-play]]
    [:span " "]
    [:button.btn {:disabled (:playing? @app-state)
                  :on-click step-action} [:i.fa.fa-step-forward]]]
   [:p (str "Step: " (:current-step @app-state) " / " (:steps @app-state))]
   [:div.slidercontainer {:style {:width (str (* d width) "px")}}
    [:input.slider {:disabled (:playing? @app-state)
                    :type "range" :min "0" :max (str (:steps @app-state))
                    :value (str (:current-step @app-state))
                    :on-change slider-action}]]])

(rd/render [app-component]
           (. js/document (getElementById "app")))

(defonce timer (js/setInterval #(swap! app-state update-state) step-time-ms))
