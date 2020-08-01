(ns game-of-life.core
  (:require [reagent.core :as r]
            [reagent.dom :as rd]))

(enable-console-print!)

(def width 100)
(def height 50)
(def d 10)
(def step-time-ms 500)

(defonce app-state (r/atom {:world (vec (repeat height (vec (repeat width 0))))
                            :update? true :steps 0}))

(swap! app-state update :world #(assoc-in % [20 24] 1))
(swap! app-state update :world #(assoc-in % [20 25] 1))
(swap! app-state update :world #(assoc-in % [20 26] 1))
(swap! app-state update :world #(assoc-in % [20 27] 1))
(swap! app-state update :world #(assoc-in % [20 28] 1))
(swap! app-state update :world #(assoc-in % [20 29] 1))
(swap! app-state update :world #(assoc-in % [20 30] 1))

(println "The Game of Life")

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
  (-> state
    (assoc :world (update-world (:world state)))
    (assoc :steps (inc (:steps state)))))

(defn update-state
  [state]
  (if (:update? state)
    (single-step-update state)
    state))

(defn render-world
  [world]
  (let [canvas (.getElementById js/document "world")
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

(defn handle-click-event
  [e]
  (let [rect (-> e .-target .getBoundingClientRect)
        t (-> rect .-top)
        l (-> rect .-left)
        r (/ (- (-> e .-clientY) t) d)
        c (/ (- (-> e .-clientX) l) d)
        _ (prn rect t l r c)]
    (toggle-cell r c)))

(defn world-component
  []
  (r/create-class
    {
     :component-did-update
     (fn [_]
       (render-world (:world @app-state)))
     :component-did-mount
     (fn [_]
       (render-world (:world @app-state)))
     :reagent-render
     (fn []
       (:world @app-state)
       [:div {:style {:width "100%"}}
        [:canvas {:id "world" :width (str (* d width)) :height (str (* d height))
                  :style {:margin "auto" :border "1px solid black"}
                  :on-click handle-click-event}]])}))

(defn app-component
  []
  [:div
   [:h1 "The Game of Life"]
   [world-component]
   [:div
    [:span "Auto-update: "]
    [:label.switch
     [:input {:type "checkbox" :checked (:update? @app-state)
              :on-change (fn [e] (swap! app-state assoc :update? (-> e .-target .-checked)))}]
     [:span.slider.round]]
    [:span {:style {:display "inline-block" :width "40px"}} " "]
    [:button.btn {:on-click #(swap! app-state single-step-update)} [:i.fa.fa-step-forward]]]
   [:p (str "Step: " (:steps @app-state))]])

(rd/render [app-component]
           (. js/document (getElementById "app")))

(js/setInterval #(swap! app-state update-state) step-time-ms)
