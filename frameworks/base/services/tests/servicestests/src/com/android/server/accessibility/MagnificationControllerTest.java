/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.server.accessibility;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyFloat;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Matchers.anyObject;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoMoreInteractions;
import static org.mockito.Mockito.when;
import static org.mockito.hamcrest.MockitoHamcrest.argThat;

import android.animation.ValueAnimator;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.IntentFilter;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.Region;
import android.os.Looper;
import android.view.MagnificationSpec;

import androidx.test.InstrumentationRegistry;
import androidx.test.runner.AndroidJUnit4;

import com.android.server.wm.WindowManagerInternal;
import com.android.server.wm.WindowManagerInternal.MagnificationCallbacks;

import org.hamcrest.CoreMatchers;
import org.hamcrest.Description;
import org.hamcrest.TypeSafeMatcher;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

import java.util.Locale;

@RunWith(AndroidJUnit4.class)
public class MagnificationControllerTest {
    static final Rect INITIAL_MAGNIFICATION_BOUNDS = new Rect(0, 0, 100, 200);
    static final PointF INITIAL_MAGNIFICATION_BOUNDS_CENTER = new PointF(
            INITIAL_MAGNIFICATION_BOUNDS.centerX(), INITIAL_MAGNIFICATION_BOUNDS.centerY());
    static final PointF INITIAL_BOUNDS_UPPER_LEFT_2X_CENTER = new PointF(25, 50);
    static final PointF INITIAL_BOUNDS_LOWER_RIGHT_2X_CENTER = new PointF(75, 150);
    static final Rect OTHER_MAGNIFICATION_BOUNDS = new Rect(100, 200, 500, 600);
    static final Rect OTHER_MAGNIFICATION_BOUNDS_COMPAT = new Rect(50, 100, 450, 500);
    static final PointF OTHER_BOUNDS_LOWER_RIGHT_2X_CENTER = new PointF(400, 500);
    static final Region INITIAL_MAGNIFICATION_REGION = new Region(INITIAL_MAGNIFICATION_BOUNDS);
    static final Region OTHER_REGION_COMPAT = new Region(OTHER_MAGNIFICATION_BOUNDS_COMPAT);
    static final Region OTHER_REGION = new Region(OTHER_MAGNIFICATION_BOUNDS);
    static final int SERVICE_ID_1 = 1;
    static final int SERVICE_ID_2 = 2;
    static final int DISPLAY_0 = 0;
    static final int DISPLAY_1 = 1;
    static final int DISPLAY_COUNT = 2;
    static final int INVALID_DISPLAY = 2;

    final MagnificationController.ControllerContext mMockControllerCtx =
            mock(MagnificationController.ControllerContext.class);
    final Context mMockContext = mock(Context.class);
    final AccessibilityManagerService mMockAms = mock(AccessibilityManagerService.class);
    final WindowManagerInternal mMockWindowManager = mock(WindowManagerInternal.class);
    final MessageCapturingHandler mMessageCapturingHandler = new MessageCapturingHandler(null);

    ValueAnimator mMockValueAnimator;
    ValueAnimator.AnimatorUpdateListener mTargetAnimationListener;

    MagnificationController mMagnificationController;

    @Before
    public void setUp() {
        Looper looper = InstrumentationRegistry.getContext().getMainLooper();
        // Pretending ID of the Thread associated with looper as main thread ID in controller
        when(mMockContext.getMainLooper()).thenReturn(looper);
        when(mMockControllerCtx.getContext()).thenReturn(mMockContext);
        when(mMockControllerCtx.getAms()).thenReturn(mMockAms);
        when(mMockControllerCtx.getWindowManager()).thenReturn(mMockWindowManager);
        when(mMockControllerCtx.getHandler()).thenReturn(mMessageCapturingHandler);
        when(mMockControllerCtx.getAnimationDuration()).thenReturn(1000L);
        initMockWindowManager();

        mMagnificationController = new MagnificationController(mMockControllerCtx, new Object());
    }

    @After
    public void tearDown() {
        mMessageCapturingHandler.removeAllMessages();
    }


    @Test
    public void testRegister_WindowManagerAndContextRegisterListeners() {
        register(DISPLAY_0);
        register(DISPLAY_1);
        register(INVALID_DISPLAY);
        verify(mMockContext).registerReceiver(
                (BroadcastReceiver) anyObject(), (IntentFilter) anyObject());
        verify(mMockWindowManager).setMagnificationCallbacks(
                eq(DISPLAY_0), (MagnificationCallbacks) anyObject());
        verify(mMockWindowManager).setMagnificationCallbacks(
                eq(DISPLAY_1), (MagnificationCallbacks) anyObject());
        verify(mMockWindowManager).setMagnificationCallbacks(
                eq(INVALID_DISPLAY), (MagnificationCallbacks) anyObject());
        assertTrue(mMagnificationController.isRegistered(DISPLAY_0));
        assertTrue(mMagnificationController.isRegistered(DISPLAY_1));
        assertFalse(mMagnificationController.isRegistered(INVALID_DISPLAY));
    }

    @Test
    public void testRegister_WindowManagerAndContextUnregisterListeners() {
        register(DISPLAY_0);
        register(DISPLAY_1);
        mMagnificationController.unregister(DISPLAY_0);
        verify(mMockContext, times(0)).unregisterReceiver((BroadcastReceiver) anyObject());
        mMagnificationController.unregister(DISPLAY_1);
        verify(mMockContext).unregisterReceiver((BroadcastReceiver) anyObject());
        verify(mMockWindowManager).setMagnificationCallbacks(eq(DISPLAY_0), eq(null));
        verify(mMockWindowManager).setMagnificationCallbacks(eq(DISPLAY_1), eq(null));
        assertFalse(mMagnificationController.isRegistered(DISPLAY_0));
        assertFalse(mMagnificationController.isRegistered(DISPLAY_1));
    }

    @Test
    public void testInitialState_noMagnificationAndMagnificationRegionReadFromWindowManager() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            initialState_noMagnificationAndMagnificationRegionReadFromWindowManager(i);
            resetMockWindowManager();
        }
    }

    private void initialState_noMagnificationAndMagnificationRegionReadFromWindowManager(
            int displayId) {
        register(displayId);
        MagnificationSpec expectedInitialSpec = getMagnificationSpec(1.0f, 0.0f, 0.0f);
        Region initialMagRegion = new Region();
        Rect initialBounds = new Rect();

        assertEquals(expectedInitialSpec, getCurrentMagnificationSpec(displayId));
        mMagnificationController.getMagnificationRegion(displayId, initialMagRegion);
        mMagnificationController.getMagnificationBounds(displayId, initialBounds);
        assertEquals(INITIAL_MAGNIFICATION_REGION, initialMagRegion);
        assertEquals(INITIAL_MAGNIFICATION_BOUNDS, initialBounds);
        assertEquals(INITIAL_MAGNIFICATION_BOUNDS.centerX(),
                mMagnificationController.getCenterX(displayId), 0.0f);
        assertEquals(INITIAL_MAGNIFICATION_BOUNDS.centerY(),
                mMagnificationController.getCenterY(displayId), 0.0f);
    }

    @Test
    public void testNotRegistered_publicMethodsShouldBeBenign() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            notRegistered_publicMethodsShouldBeBenign(i);
            resetMockWindowManager();
        }
    }

    private void notRegistered_publicMethodsShouldBeBenign(int displayId) {
        assertFalse(mMagnificationController.isMagnifying(displayId));
        assertFalse(mMagnificationController.magnificationRegionContains(displayId, 100, 100));
        assertFalse(mMagnificationController.reset(displayId, true));
        assertFalse(mMagnificationController.setScale(displayId, 2, 100, 100, true, 0));
        assertFalse(mMagnificationController.setCenter(displayId, 100, 100, false, 1));
        assertFalse(mMagnificationController.setScaleAndCenter(displayId,
                1.5f, 100, 100, false, 2));
        assertTrue(mMagnificationController.getIdOfLastServiceToMagnify(displayId) < 0);

        mMagnificationController.getMagnificationRegion(displayId, new Region());
        mMagnificationController.getMagnificationBounds(displayId, new Rect());
        mMagnificationController.getScale(displayId);
        mMagnificationController.getOffsetX(displayId);
        mMagnificationController.getOffsetY(displayId);
        mMagnificationController.getCenterX(displayId);
        mMagnificationController.getCenterY(displayId);
        mMagnificationController.offsetMagnifiedRegion(displayId, 50, 50, 1);
        mMagnificationController.unregister(displayId);
    }

    @Test
    public void testSetScale_noAnimation_shouldGoStraightToWindowManagerAndUpdateState() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            setScale_noAnimation_shouldGoStraightToWindowManagerAndUpdateState(i);
            resetMockWindowManager();
        }
    }

    private void setScale_noAnimation_shouldGoStraightToWindowManagerAndUpdateState(int displayId) {
        register(displayId);
        final float scale = 2.0f;
        final PointF center = INITIAL_MAGNIFICATION_BOUNDS_CENTER;
        final PointF offsets = computeOffsets(INITIAL_MAGNIFICATION_BOUNDS, center, scale);
        assertTrue(mMagnificationController
                .setScale(displayId, scale, center.x, center.y, false, SERVICE_ID_1));
        mMessageCapturingHandler.sendAllMessages();

        final MagnificationSpec expectedSpec = getMagnificationSpec(scale, offsets);
        verify(mMockWindowManager).setMagnificationSpec(
                eq(displayId), argThat(closeTo(expectedSpec)));
        assertThat(getCurrentMagnificationSpec(displayId), closeTo(expectedSpec));
        assertEquals(center.x, mMagnificationController.getCenterX(displayId), 0.0);
        assertEquals(center.y, mMagnificationController.getCenterY(displayId), 0.0);
        verify(mMockValueAnimator, times(0)).start();
    }

    @Test
    public void testSetScale_withPivotAndAnimation_stateChangesAndAnimationHappens() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            setScale_withPivotAndAnimation_stateChangesAndAnimationHappens(i);
            resetMockWindowManager();
        }
    }

    private void setScale_withPivotAndAnimation_stateChangesAndAnimationHappens(int displayId) {
        register(displayId);
        MagnificationSpec startSpec = getCurrentMagnificationSpec(displayId);
        float scale = 2.0f;
        PointF pivotPoint = INITIAL_BOUNDS_LOWER_RIGHT_2X_CENTER;
        assertTrue(mMagnificationController
                .setScale(displayId, scale, pivotPoint.x, pivotPoint.y, true, SERVICE_ID_1));
        mMessageCapturingHandler.sendAllMessages();

        // New center should be halfway between original center and pivot
        PointF newCenter = new PointF((pivotPoint.x + INITIAL_MAGNIFICATION_BOUNDS.centerX()) / 2,
                (pivotPoint.y + INITIAL_MAGNIFICATION_BOUNDS.centerY()) / 2);
        PointF offsets = computeOffsets(INITIAL_MAGNIFICATION_BOUNDS, newCenter, scale);
        MagnificationSpec endSpec = getMagnificationSpec(scale, offsets);

        assertEquals(newCenter.x, mMagnificationController.getCenterX(displayId), 0.5);
        assertEquals(newCenter.y, mMagnificationController.getCenterY(displayId), 0.5);
        assertThat(getCurrentMagnificationSpec(displayId), closeTo(endSpec));
        verify(mMockValueAnimator).start();

        // Initial value
        when(mMockValueAnimator.getAnimatedFraction()).thenReturn(0.0f);
        mTargetAnimationListener.onAnimationUpdate(mMockValueAnimator);
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId), eq(startSpec));

        // Intermediate point
        Mockito.reset(mMockWindowManager);
        float fraction = 0.5f;
        when(mMockValueAnimator.getAnimatedFraction()).thenReturn(fraction);
        mTargetAnimationListener.onAnimationUpdate(mMockValueAnimator);
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId),
                argThat(closeTo(getInterpolatedMagSpec(startSpec, endSpec, fraction))));

        // Final value
        Mockito.reset(mMockWindowManager);
        when(mMockValueAnimator.getAnimatedFraction()).thenReturn(1.0f);
        mTargetAnimationListener.onAnimationUpdate(mMockValueAnimator);
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId), argThat(closeTo(endSpec)));
    }

    @Test
    public void testSetCenter_whileMagnifying_noAnimation_centerMoves() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            setCenter_whileMagnifying_noAnimation_centerMoves(i);
            resetMockWindowManager();
        }
    }

    private void setCenter_whileMagnifying_noAnimation_centerMoves(int displayId) {
        register(displayId);
        // First zoom in
        float scale = 2.0f;
        assertTrue(mMagnificationController.setScale(displayId, scale,
                INITIAL_MAGNIFICATION_BOUNDS.centerX(), INITIAL_MAGNIFICATION_BOUNDS.centerY(),
                false, SERVICE_ID_1));
        Mockito.reset(mMockWindowManager);

        PointF newCenter = INITIAL_BOUNDS_LOWER_RIGHT_2X_CENTER;
        assertTrue(mMagnificationController
                .setCenter(displayId, newCenter.x, newCenter.y, false, SERVICE_ID_1));
        mMessageCapturingHandler.sendAllMessages();
        PointF expectedOffsets = computeOffsets(INITIAL_MAGNIFICATION_BOUNDS, newCenter, scale);
        MagnificationSpec expectedSpec = getMagnificationSpec(scale, expectedOffsets);

        verify(mMockWindowManager).setMagnificationSpec(
                eq(displayId), argThat(closeTo(expectedSpec)));
        assertEquals(newCenter.x, mMagnificationController.getCenterX(displayId), 0.0);
        assertEquals(newCenter.y, mMagnificationController.getCenterY(displayId), 0.0);
        verify(mMockValueAnimator, times(0)).start();
    }

    @Test
    public void testSetScaleAndCenter_animated_stateChangesAndAnimationHappens() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            setScaleAndCenter_animated_stateChangesAndAnimationHappens(i);
            resetMockWindowManager();
        }
    }

    private void setScaleAndCenter_animated_stateChangesAndAnimationHappens(int displayId) {
        register(displayId);
        MagnificationSpec startSpec = getCurrentMagnificationSpec(displayId);
        float scale = 2.5f;
        PointF newCenter = INITIAL_BOUNDS_LOWER_RIGHT_2X_CENTER;
        PointF offsets = computeOffsets(INITIAL_MAGNIFICATION_BOUNDS, newCenter, scale);
        MagnificationSpec endSpec = getMagnificationSpec(scale, offsets);

        assertTrue(mMagnificationController.setScaleAndCenter(displayId, scale, newCenter.x,
                newCenter.y, true, SERVICE_ID_1));
        mMessageCapturingHandler.sendAllMessages();

        assertEquals(newCenter.x, mMagnificationController.getCenterX(displayId), 0.5);
        assertEquals(newCenter.y, mMagnificationController.getCenterY(displayId), 0.5);
        assertThat(getCurrentMagnificationSpec(displayId), closeTo(endSpec));
        verify(mMockAms).notifyMagnificationChanged(displayId,
                INITIAL_MAGNIFICATION_REGION, scale, newCenter.x, newCenter.y);
        verify(mMockValueAnimator).start();

        // Initial value
        when(mMockValueAnimator.getAnimatedFraction()).thenReturn(0.0f);
        mTargetAnimationListener.onAnimationUpdate(mMockValueAnimator);
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId), eq(startSpec));

        // Intermediate point
        Mockito.reset(mMockWindowManager);
        float fraction = 0.33f;
        when(mMockValueAnimator.getAnimatedFraction()).thenReturn(fraction);
        mTargetAnimationListener.onAnimationUpdate(mMockValueAnimator);
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId),
                argThat(closeTo(getInterpolatedMagSpec(startSpec, endSpec, fraction))));

        // Final value
        Mockito.reset(mMockWindowManager);
        when(mMockValueAnimator.getAnimatedFraction()).thenReturn(1.0f);
        mTargetAnimationListener.onAnimationUpdate(mMockValueAnimator);
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId), argThat(closeTo(endSpec)));
    }

    @Test
    public void testSetScaleAndCenter_scaleOutOfBounds_cappedAtLimits() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            setScaleAndCenter_scaleOutOfBounds_cappedAtLimits(i);
            resetMockWindowManager();
        }
    }

    private void setScaleAndCenter_scaleOutOfBounds_cappedAtLimits(int displayId) {
        register(displayId);
        MagnificationSpec startSpec = getCurrentMagnificationSpec(displayId);
        PointF newCenter = INITIAL_BOUNDS_LOWER_RIGHT_2X_CENTER;
        PointF offsets = computeOffsets(INITIAL_MAGNIFICATION_BOUNDS, newCenter,
                MagnificationController.MAX_SCALE);
        MagnificationSpec endSpec = getMagnificationSpec(
                MagnificationController.MAX_SCALE, offsets);

        assertTrue(mMagnificationController.setScaleAndCenter(displayId,
                MagnificationController.MAX_SCALE + 1.0f,
                newCenter.x, newCenter.y, false, SERVICE_ID_1));
        mMessageCapturingHandler.sendAllMessages();

        assertEquals(newCenter.x, mMagnificationController.getCenterX(displayId), 0.5);
        assertEquals(newCenter.y, mMagnificationController.getCenterY(displayId), 0.5);
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId), argThat(closeTo(endSpec)));
        Mockito.reset(mMockWindowManager);

        // Verify that we can't zoom below 1x
        assertTrue(mMagnificationController.setScaleAndCenter(displayId, 0.5f,
                INITIAL_MAGNIFICATION_BOUNDS_CENTER.x, INITIAL_MAGNIFICATION_BOUNDS_CENTER.y,
                false, SERVICE_ID_1));
        mMessageCapturingHandler.sendAllMessages();

        assertEquals(INITIAL_MAGNIFICATION_BOUNDS_CENTER.x,
                mMagnificationController.getCenterX(displayId), 0.5);
        assertEquals(INITIAL_MAGNIFICATION_BOUNDS_CENTER.y,
                mMagnificationController.getCenterY(displayId), 0.5);
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId), argThat(closeTo(startSpec)));
    }

    @Test
    public void testSetScaleAndCenter_centerOutOfBounds_cappedAtLimits() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            setScaleAndCenter_centerOutOfBounds_cappedAtLimits(i);
            resetMockWindowManager();
        }
    }

    private void setScaleAndCenter_centerOutOfBounds_cappedAtLimits(int displayId) {
        register(displayId);
        float scale = 2.0f;

        // Off the edge to the top and left
        assertTrue(mMagnificationController.setScaleAndCenter(displayId,
                scale, -100f, -200f, false, SERVICE_ID_1));
        mMessageCapturingHandler.sendAllMessages();

        PointF newCenter = INITIAL_BOUNDS_UPPER_LEFT_2X_CENTER;
        PointF newOffsets = computeOffsets(INITIAL_MAGNIFICATION_BOUNDS, newCenter, scale);
        assertEquals(newCenter.x, mMagnificationController.getCenterX(displayId), 0.5);
        assertEquals(newCenter.y, mMagnificationController.getCenterY(displayId), 0.5);
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId),
                argThat(closeTo(getMagnificationSpec(scale, newOffsets))));
        Mockito.reset(mMockWindowManager);

        // Off the edge to the bottom and right
        assertTrue(mMagnificationController.setScaleAndCenter(displayId, scale,
                INITIAL_MAGNIFICATION_BOUNDS.right + 1, INITIAL_MAGNIFICATION_BOUNDS.bottom + 1,
                false, SERVICE_ID_1));
        mMessageCapturingHandler.sendAllMessages();
        newCenter = INITIAL_BOUNDS_LOWER_RIGHT_2X_CENTER;
        newOffsets = computeOffsets(INITIAL_MAGNIFICATION_BOUNDS, newCenter, scale);
        assertEquals(newCenter.x, mMagnificationController.getCenterX(displayId), 0.5);
        assertEquals(newCenter.y, mMagnificationController.getCenterY(displayId), 0.5);
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId),
                argThat(closeTo(getMagnificationSpec(scale, newOffsets))));
    }

    @Test
    public void testMagnificationRegionChanged_serviceNotified() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            magnificationRegionChanged_serviceNotified(i);
            resetMockWindowManager();
        }
    }

    private void magnificationRegionChanged_serviceNotified(int displayId) {
        register(displayId);
        MagnificationCallbacks callbacks = getMagnificationCallbacks(displayId);
        callbacks.onMagnificationRegionChanged(OTHER_REGION);
        mMessageCapturingHandler.sendAllMessages();
        verify(mMockAms).notifyMagnificationChanged(displayId, OTHER_REGION, 1.0f,
                OTHER_MAGNIFICATION_BOUNDS.centerX(), OTHER_MAGNIFICATION_BOUNDS.centerY());
    }

    @Test
    public void testOffsetMagnifiedRegion_whileMagnifying_offsetsMove() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            offsetMagnifiedRegion_whileMagnifying_offsetsMove(i);
            resetMockWindowManager();
        }
    }

    private void offsetMagnifiedRegion_whileMagnifying_offsetsMove(int displayId) {
        register(displayId);
        PointF startCenter = INITIAL_MAGNIFICATION_BOUNDS_CENTER;
        float scale = 2.0f;
        PointF startOffsets = computeOffsets(INITIAL_MAGNIFICATION_BOUNDS, startCenter, scale);
        // First zoom in
        assertTrue(mMagnificationController
                .setScaleAndCenter(displayId, scale, startCenter.x, startCenter.y, false,
                        SERVICE_ID_1));
        mMessageCapturingHandler.sendAllMessages();
        Mockito.reset(mMockWindowManager);

        PointF newCenter = INITIAL_BOUNDS_LOWER_RIGHT_2X_CENTER;
        PointF newOffsets = computeOffsets(INITIAL_MAGNIFICATION_BOUNDS, newCenter, scale);
        mMagnificationController.offsetMagnifiedRegion(displayId,
                startOffsets.x - newOffsets.x, startOffsets.y - newOffsets.y,
                SERVICE_ID_1);
        mMessageCapturingHandler.sendAllMessages();

        MagnificationSpec expectedSpec = getMagnificationSpec(scale, newOffsets);
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId),
                argThat(closeTo(expectedSpec)));
        assertEquals(newCenter.x, mMagnificationController.getCenterX(displayId), 0.0);
        assertEquals(newCenter.y, mMagnificationController.getCenterY(displayId), 0.0);
        verify(mMockValueAnimator, times(0)).start();
    }

    @Test
    public void testOffsetMagnifiedRegion_whileNotMagnifying_hasNoEffect() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            offsetMagnifiedRegion_whileNotMagnifying_hasNoEffect(i);
            resetMockWindowManager();
        }
    }

    private void offsetMagnifiedRegion_whileNotMagnifying_hasNoEffect(int displayId) {
        register(displayId);
        Mockito.reset(mMockWindowManager);
        MagnificationSpec startSpec = getCurrentMagnificationSpec(displayId);
        mMagnificationController.offsetMagnifiedRegion(displayId, 10, 10, SERVICE_ID_1);
        assertThat(getCurrentMagnificationSpec(displayId), closeTo(startSpec));
        mMagnificationController.offsetMagnifiedRegion(displayId, -10, -10, SERVICE_ID_1);
        assertThat(getCurrentMagnificationSpec(displayId), closeTo(startSpec));
        verifyNoMoreInteractions(mMockWindowManager);
    }

    @Test
    public void testOffsetMagnifiedRegion_whileMagnifyingButAtEdge_hasNoEffect() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            offsetMagnifiedRegion_whileMagnifyingButAtEdge_hasNoEffect(i);
            resetMockWindowManager();
        }
    }

    private void offsetMagnifiedRegion_whileMagnifyingButAtEdge_hasNoEffect(int displayId) {
        register(displayId);
        float scale = 2.0f;

        // Upper left edges
        PointF ulCenter = INITIAL_BOUNDS_UPPER_LEFT_2X_CENTER;
        assertTrue(mMagnificationController
                .setScaleAndCenter(displayId, scale, ulCenter.x, ulCenter.y, false,
                        SERVICE_ID_1));
        Mockito.reset(mMockWindowManager);
        MagnificationSpec ulSpec = getCurrentMagnificationSpec(displayId);
        mMagnificationController.offsetMagnifiedRegion(displayId, -10, -10,
                SERVICE_ID_1);
        assertThat(getCurrentMagnificationSpec(displayId), closeTo(ulSpec));
        verifyNoMoreInteractions(mMockWindowManager);

        // Lower right edges
        PointF lrCenter = INITIAL_BOUNDS_LOWER_RIGHT_2X_CENTER;
        assertTrue(mMagnificationController
                .setScaleAndCenter(displayId, scale, lrCenter.x, lrCenter.y, false,
                        SERVICE_ID_1));
        Mockito.reset(mMockWindowManager);
        MagnificationSpec lrSpec = getCurrentMagnificationSpec(displayId);
        mMagnificationController.offsetMagnifiedRegion(displayId, 10, 10,
                SERVICE_ID_1);
        assertThat(getCurrentMagnificationSpec(displayId), closeTo(lrSpec));
        verifyNoMoreInteractions(mMockWindowManager);
    }

    @Test
    public void testGetIdOfLastServiceToChange_returnsCorrectValue() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            getIdOfLastServiceToChange_returnsCorrectValue(i);
            resetMockWindowManager();
        }
    }

    private void getIdOfLastServiceToChange_returnsCorrectValue(int displayId) {
        register(displayId);
        PointF startCenter = INITIAL_MAGNIFICATION_BOUNDS_CENTER;
        assertTrue(mMagnificationController
                .setScale(displayId, 2.0f, startCenter.x, startCenter.y, false,
                        SERVICE_ID_1));
        assertEquals(SERVICE_ID_1, mMagnificationController.getIdOfLastServiceToMagnify(displayId));
        assertTrue(mMagnificationController
                .setScale(displayId, 1.5f, startCenter.x, startCenter.y, false,
                        SERVICE_ID_2));
        assertEquals(SERVICE_ID_2, mMagnificationController.getIdOfLastServiceToMagnify(displayId));
    }

    @Test
    public void testResetIfNeeded_resetsOnlyIfLastMagnifyingServiceIsDisabled() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            resetIfNeeded_resetsOnlyIfLastMagnifyingServiceIsDisabled(i);
            resetMockWindowManager();
        }
    }

    private void resetIfNeeded_resetsOnlyIfLastMagnifyingServiceIsDisabled(int displayId) {
        register(displayId);
        PointF startCenter = INITIAL_MAGNIFICATION_BOUNDS_CENTER;
        mMagnificationController
                .setScale(displayId, 2.0f, startCenter.x, startCenter.y, false,
                        SERVICE_ID_1);
        mMagnificationController
                .setScale(displayId, 1.5f, startCenter.x, startCenter.y, false,
                        SERVICE_ID_2);
        assertFalse(mMagnificationController.resetIfNeeded(displayId, SERVICE_ID_1));
        assertTrue(mMagnificationController.isMagnifying(displayId));
        assertTrue(mMagnificationController.resetIfNeeded(displayId, SERVICE_ID_2));
        assertFalse(mMagnificationController.isMagnifying(displayId));
    }

    @Test
    public void testSetUserId_resetsOnlyIfIdChanges() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            testSetUserId_resetsOnlyIfIdChanges(i);
            resetMockWindowManager();
        }
    }

    private void testSetUserId_resetsOnlyIfIdChanges(int displayId) {
        final int userId1 = 1;
        final int userId2 = 2;

        register(displayId);
        mMagnificationController.setUserId(userId1);
        PointF startCenter = INITIAL_MAGNIFICATION_BOUNDS_CENTER;
        float scale = 2.0f;
        mMagnificationController.setScale(displayId, scale, startCenter.x, startCenter.y, false,
                SERVICE_ID_1);

        mMagnificationController.setUserId(userId1);
        assertTrue(mMagnificationController.isMagnifying(displayId));
        mMagnificationController.setUserId(userId2);
        assertFalse(mMagnificationController.isMagnifying(displayId));
    }

    @Test
    public void testResetIfNeeded_doesWhatItSays() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            testResetIfNeeded_doesWhatItSays(i);
            resetMockWindowManager();
        }
    }

    private void testResetIfNeeded_doesWhatItSays(int displayId) {
        register(displayId);
        zoomIn2xToMiddle(displayId);
        mMessageCapturingHandler.sendAllMessages();
        reset(mMockAms);
        assertTrue(mMagnificationController.resetIfNeeded(displayId, false));
        verify(mMockAms).notifyMagnificationChanged(eq(displayId),
                eq(INITIAL_MAGNIFICATION_REGION), eq(1.0f), anyFloat(), anyFloat());
        assertFalse(mMagnificationController.isMagnifying(displayId));
        assertFalse(mMagnificationController.resetIfNeeded(displayId, false));
    }

    @Test
    public void testTurnScreenOff_resetsMagnification() {
        register(DISPLAY_0);
        register(DISPLAY_1);
        ArgumentCaptor<BroadcastReceiver> broadcastReceiverCaptor =
                ArgumentCaptor.forClass(BroadcastReceiver.class);
        verify(mMockContext).registerReceiver(
                broadcastReceiverCaptor.capture(), (IntentFilter) anyObject());
        BroadcastReceiver br = broadcastReceiverCaptor.getValue();
        zoomIn2xToMiddle(DISPLAY_0);
        zoomIn2xToMiddle(DISPLAY_1);
        mMessageCapturingHandler.sendAllMessages();
        br.onReceive(mMockContext, null);
        mMessageCapturingHandler.sendAllMessages();
        assertFalse(mMagnificationController.isMagnifying(DISPLAY_0));
        assertFalse(mMagnificationController.isMagnifying(DISPLAY_1));
    }

    @Test
    public void testUserContextChange_resetsMagnification() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            contextChange_resetsMagnification(i);
            resetMockWindowManager();
        }
    }

    private void contextChange_resetsMagnification(int displayId) {
        register(displayId);
        MagnificationCallbacks callbacks = getMagnificationCallbacks(displayId);
        zoomIn2xToMiddle(displayId);
        mMessageCapturingHandler.sendAllMessages();
        callbacks.onUserContextChanged();
        mMessageCapturingHandler.sendAllMessages();
        assertFalse(mMagnificationController.isMagnifying(displayId));
    }

    @Test
    public void testRotation_resetsMagnification() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            rotation_resetsMagnification(i);
            resetMockWindowManager();
        }
    }

    private void rotation_resetsMagnification(int displayId) {
        register(displayId);
        MagnificationCallbacks callbacks = getMagnificationCallbacks(displayId);
        zoomIn2xToMiddle(displayId);
        mMessageCapturingHandler.sendAllMessages();
        assertTrue(mMagnificationController.isMagnifying(displayId));
        callbacks.onRotationChanged(0);
        mMessageCapturingHandler.sendAllMessages();
        assertFalse(mMagnificationController.isMagnifying(displayId));
    }

    @Test
    public void testBoundsChange_whileMagnifyingWithCompatibleSpec_noSpecChange() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            boundsChange_whileMagnifyingWithCompatibleSpec_noSpecChange(i);
            resetMockWindowManager();
        }
    }

    private void boundsChange_whileMagnifyingWithCompatibleSpec_noSpecChange(int displayId) {
        // Going from a small region to a large one leads to no issues
        register(displayId);
        zoomIn2xToMiddle(displayId);
        mMessageCapturingHandler.sendAllMessages();
        MagnificationSpec startSpec = getCurrentMagnificationSpec(displayId);
        MagnificationCallbacks callbacks = getMagnificationCallbacks(displayId);
        Mockito.reset(mMockWindowManager);
        callbacks.onMagnificationRegionChanged(OTHER_REGION_COMPAT);
        mMessageCapturingHandler.sendAllMessages();
        assertThat(getCurrentMagnificationSpec(displayId), closeTo(startSpec));
        verifyNoMoreInteractions(mMockWindowManager);
    }

    @Test
    public void testBoundsChange_whileZoomingWithCompatibleSpec_noSpecChange() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            boundsChange_whileZoomingWithCompatibleSpec_noSpecChange(i);
            resetMockWindowManager();
        }
    }

    private void boundsChange_whileZoomingWithCompatibleSpec_noSpecChange(int displayId) {
        register(displayId);
        PointF startCenter = INITIAL_MAGNIFICATION_BOUNDS_CENTER;
        float scale = 2.0f;
        // setting animate parameter to true is differ from zoomIn2xToMiddle()
        mMagnificationController.setScale(displayId, scale, startCenter.x, startCenter.y, true,
                SERVICE_ID_1);
        MagnificationSpec startSpec = getCurrentMagnificationSpec(displayId);
        MagnificationCallbacks callbacks = getMagnificationCallbacks(displayId);
        Mockito.reset(mMockWindowManager);
        callbacks.onMagnificationRegionChanged(OTHER_REGION_COMPAT);
        mMessageCapturingHandler.sendAllMessages();
        assertThat(getCurrentMagnificationSpec(displayId), closeTo(startSpec));
        verifyNoMoreInteractions(mMockWindowManager);
    }

    @Test
    public void testBoundsChange_whileMagnifyingWithIncompatibleSpec_offsetsConstrained() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            boundsChange_whileMagnifyingWithIncompatibleSpec_offsetsConstrained(i);
            resetMockWindowManager();
        }
    }

    private void boundsChange_whileMagnifyingWithIncompatibleSpec_offsetsConstrained(
            int displayId) {
        // In a large region, pan to the farthest point possible
        register(displayId);
        MagnificationCallbacks callbacks = getMagnificationCallbacks(displayId);
        callbacks.onMagnificationRegionChanged(OTHER_REGION);
        mMessageCapturingHandler.sendAllMessages();
        PointF startCenter = OTHER_BOUNDS_LOWER_RIGHT_2X_CENTER;
        float scale = 2.0f;
        mMagnificationController.setScale(displayId, scale, startCenter.x, startCenter.y, false,
                SERVICE_ID_1);
        mMessageCapturingHandler.sendAllMessages();
        MagnificationSpec startSpec = getCurrentMagnificationSpec(displayId);
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId), argThat(closeTo(startSpec)));
        Mockito.reset(mMockWindowManager);

        callbacks.onMagnificationRegionChanged(INITIAL_MAGNIFICATION_REGION);
        mMessageCapturingHandler.sendAllMessages();

        MagnificationSpec endSpec = getCurrentMagnificationSpec(displayId);
        assertThat(endSpec, CoreMatchers.not(closeTo(startSpec)));
        PointF expectedOffsets = computeOffsets(INITIAL_MAGNIFICATION_BOUNDS,
                INITIAL_BOUNDS_LOWER_RIGHT_2X_CENTER, scale);
        assertThat(endSpec, closeTo(getMagnificationSpec(scale, expectedOffsets)));
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId), argThat(closeTo(endSpec)));
    }

    @Test
    public void testBoundsChange_whileZoomingWithIncompatibleSpec_jumpsToCompatibleSpec() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            boundsChange_whileZoomingWithIncompatibleSpec_jumpsToCompatibleSpec(i);
            resetMockWindowManager();
        }
    }

    private void boundsChange_whileZoomingWithIncompatibleSpec_jumpsToCompatibleSpec(
            int displayId) {
        register(displayId);
        MagnificationCallbacks callbacks = getMagnificationCallbacks(displayId);
        callbacks.onMagnificationRegionChanged(OTHER_REGION);
        mMessageCapturingHandler.sendAllMessages();
        PointF startCenter = OTHER_BOUNDS_LOWER_RIGHT_2X_CENTER;
        float scale = 2.0f;
        mMagnificationController.setScale(displayId, scale, startCenter.x, startCenter.y, true,
                SERVICE_ID_1);
        mMessageCapturingHandler.sendAllMessages();
        MagnificationSpec startSpec = getCurrentMagnificationSpec(displayId);
        when(mMockValueAnimator.isRunning()).thenReturn(true);

        callbacks.onMagnificationRegionChanged(INITIAL_MAGNIFICATION_REGION);
        mMessageCapturingHandler.sendAllMessages();
        verify(mMockValueAnimator).cancel();

        MagnificationSpec endSpec = getCurrentMagnificationSpec(displayId);
        assertThat(endSpec, CoreMatchers.not(closeTo(startSpec)));
        PointF expectedOffsets = computeOffsets(INITIAL_MAGNIFICATION_BOUNDS,
                INITIAL_BOUNDS_LOWER_RIGHT_2X_CENTER, scale);
        assertThat(endSpec, closeTo(getMagnificationSpec(scale, expectedOffsets)));
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId), argThat(closeTo(endSpec)));
    }

    @Test
    public void testRequestRectOnScreen_rectAlreadyOnScreen_doesNothing() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            requestRectOnScreen_rectAlreadyOnScreen_doesNothing(i);
            resetMockWindowManager();
        }
    }

    private void requestRectOnScreen_rectAlreadyOnScreen_doesNothing(int displayId) {
        register(displayId);
        zoomIn2xToMiddle(displayId);
        mMessageCapturingHandler.sendAllMessages();
        MagnificationSpec startSpec = getCurrentMagnificationSpec(displayId);
        MagnificationCallbacks callbacks = getMagnificationCallbacks(displayId);
        Mockito.reset(mMockWindowManager);
        int centerX = (int) INITIAL_MAGNIFICATION_BOUNDS_CENTER.x;
        int centerY = (int) INITIAL_MAGNIFICATION_BOUNDS_CENTER.y;
        callbacks.onRectangleOnScreenRequested(centerX - 1, centerY - 1, centerX + 1, centerY - 1);
        mMessageCapturingHandler.sendAllMessages();
        assertThat(getCurrentMagnificationSpec(displayId), closeTo(startSpec));
        verifyNoMoreInteractions(mMockWindowManager);
    }

    @Test
    public void testRequestRectOnScreen_rectCanFitOnScreen_pansToGetRectOnScreen() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            requestRectOnScreen_rectCanFitOnScreen_pansToGetRectOnScreen(i);
            resetMockWindowManager();
        }
    }

    private void requestRectOnScreen_rectCanFitOnScreen_pansToGetRectOnScreen(int displayId) {
        register(displayId);
        zoomIn2xToMiddle(displayId);
        mMessageCapturingHandler.sendAllMessages();
        MagnificationCallbacks callbacks = getMagnificationCallbacks(displayId);
        Mockito.reset(mMockWindowManager);
        callbacks.onRectangleOnScreenRequested(0, 0, 1, 1);
        mMessageCapturingHandler.sendAllMessages();
        MagnificationSpec expectedEndSpec = getMagnificationSpec(2.0f, 0, 0);
        assertThat(getCurrentMagnificationSpec(displayId), closeTo(expectedEndSpec));
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId),
                argThat(closeTo(expectedEndSpec)));
    }

    @Test
    public void testRequestRectOnScreen_garbageInput_doesNothing() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            requestRectOnScreen_garbageInput_doesNothing(i);
            resetMockWindowManager();
        }
    }

    private void requestRectOnScreen_garbageInput_doesNothing(int displayId) {
        register(displayId);
        zoomIn2xToMiddle(displayId);
        mMessageCapturingHandler.sendAllMessages();
        MagnificationSpec startSpec = getCurrentMagnificationSpec(displayId);
        MagnificationCallbacks callbacks = getMagnificationCallbacks(displayId);
        Mockito.reset(mMockWindowManager);
        callbacks.onRectangleOnScreenRequested(0, 0, -50, -50);
        mMessageCapturingHandler.sendAllMessages();
        assertThat(getCurrentMagnificationSpec(displayId), closeTo(startSpec));
        verifyNoMoreInteractions(mMockWindowManager);
    }

    @Test
    public void testRequestRectOnScreen_rectTooWide_pansToGetStartOnScreenBasedOnLocale() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            requestRectOnScreen_rectTooWide_pansToGetStartOnScreenBasedOnLocale(i);
            resetMockWindowManager();
        }
    }

    private void requestRectOnScreen_rectTooWide_pansToGetStartOnScreenBasedOnLocale(
            int displayId) {
        Locale.setDefault(new Locale("en", "us"));
        register(displayId);
        zoomIn2xToMiddle(displayId);
        mMessageCapturingHandler.sendAllMessages();
        MagnificationCallbacks callbacks = getMagnificationCallbacks(displayId);
        MagnificationSpec startSpec = getCurrentMagnificationSpec(displayId);
        Mockito.reset(mMockWindowManager);
        Rect wideRect = new Rect(0, 50, 100, 51);
        callbacks.onRectangleOnScreenRequested(
                wideRect.left, wideRect.top, wideRect.right, wideRect.bottom);
        mMessageCapturingHandler.sendAllMessages();
        MagnificationSpec expectedEndSpec = getMagnificationSpec(2.0f, 0, startSpec.offsetY);
        assertThat(getCurrentMagnificationSpec(displayId), closeTo(expectedEndSpec));
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId),
                argThat(closeTo(expectedEndSpec)));
        Mockito.reset(mMockWindowManager);

        // Repeat with RTL
        Locale.setDefault(new Locale("he", "il"));
        callbacks.onRectangleOnScreenRequested(
                wideRect.left, wideRect.top, wideRect.right, wideRect.bottom);
        mMessageCapturingHandler.sendAllMessages();
        expectedEndSpec = getMagnificationSpec(2.0f, -100, startSpec.offsetY);
        assertThat(getCurrentMagnificationSpec(displayId), closeTo(expectedEndSpec));
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId),
                argThat(closeTo(expectedEndSpec)));
    }

    @Test
    public void testRequestRectOnScreen_rectTooTall_pansMinimumToGetTopOnScreen() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            requestRectOnScreen_rectTooTall_pansMinimumToGetTopOnScreen(i);
            resetMockWindowManager();
        }
    }

    private void requestRectOnScreen_rectTooTall_pansMinimumToGetTopOnScreen(int displayId) {
        register(displayId);
        zoomIn2xToMiddle(displayId);
        mMessageCapturingHandler.sendAllMessages();
        MagnificationCallbacks callbacks = getMagnificationCallbacks(displayId);
        MagnificationSpec startSpec = getCurrentMagnificationSpec(displayId);
        Mockito.reset(mMockWindowManager);
        Rect tallRect = new Rect(50, 0, 51, 100);
        callbacks.onRectangleOnScreenRequested(
                tallRect.left, tallRect.top, tallRect.right, tallRect.bottom);
        mMessageCapturingHandler.sendAllMessages();
        MagnificationSpec expectedEndSpec = getMagnificationSpec(2.0f, startSpec.offsetX, 0);
        assertThat(getCurrentMagnificationSpec(displayId), closeTo(expectedEndSpec));
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId),
                argThat(closeTo(expectedEndSpec)));
    }

    @Test
    public void testChangeMagnification_duringAnimation_animatesToNewValue() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            changeMagnification_duringAnimation_animatesToNewValue(i);
            resetMockWindowManager();
        }
    }

    private void changeMagnification_duringAnimation_animatesToNewValue(int displayId) {
        register(displayId);
        MagnificationSpec startSpec = getCurrentMagnificationSpec(displayId);
        float scale = 2.5f;
        PointF firstCenter = INITIAL_BOUNDS_LOWER_RIGHT_2X_CENTER;
        MagnificationSpec firstEndSpec = getMagnificationSpec(
                scale, computeOffsets(INITIAL_MAGNIFICATION_BOUNDS, firstCenter, scale));

        assertTrue(mMagnificationController.setScaleAndCenter(displayId,
                scale, firstCenter.x, firstCenter.y, true, SERVICE_ID_1));
        mMessageCapturingHandler.sendAllMessages();

        assertEquals(firstCenter.x, mMagnificationController.getCenterX(displayId), 0.5);
        assertEquals(firstCenter.y, mMagnificationController.getCenterY(displayId), 0.5);
        assertThat(getCurrentMagnificationSpec(displayId), closeTo(firstEndSpec));
        verify(mMockValueAnimator, times(1)).start();

        // Initial value
        when(mMockValueAnimator.getAnimatedFraction()).thenReturn(0.0f);
        mTargetAnimationListener.onAnimationUpdate(mMockValueAnimator);
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId), eq(startSpec));
        verify(mMockAms).notifyMagnificationChanged(displayId,
                INITIAL_MAGNIFICATION_REGION, scale, firstCenter.x, firstCenter.y);
        Mockito.reset(mMockWindowManager);

        // Intermediate point
        float fraction = 0.33f;
        when(mMockValueAnimator.getAnimatedFraction()).thenReturn(fraction);
        mTargetAnimationListener.onAnimationUpdate(mMockValueAnimator);
        MagnificationSpec intermediateSpec1 =
                getInterpolatedMagSpec(startSpec, firstEndSpec, fraction);
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId),
                argThat(closeTo(intermediateSpec1)));
        Mockito.reset(mMockWindowManager);

        PointF newCenter = INITIAL_BOUNDS_UPPER_LEFT_2X_CENTER;
        MagnificationSpec newEndSpec = getMagnificationSpec(
                scale, computeOffsets(INITIAL_MAGNIFICATION_BOUNDS, newCenter, scale));
        assertTrue(mMagnificationController.setCenter(displayId,
                newCenter.x, newCenter.y, true, SERVICE_ID_1));
        mMessageCapturingHandler.sendAllMessages();

        // Animation should have been restarted
        verify(mMockValueAnimator, times(2)).start();
        verify(mMockAms).notifyMagnificationChanged(displayId,
                INITIAL_MAGNIFICATION_REGION, scale, newCenter.x, newCenter.y);

        // New starting point should be where we left off
        when(mMockValueAnimator.getAnimatedFraction()).thenReturn(0.0f);
        mTargetAnimationListener.onAnimationUpdate(mMockValueAnimator);
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId),
                argThat(closeTo(intermediateSpec1)));
        Mockito.reset(mMockWindowManager);

        // Second intermediate point
        fraction = 0.5f;
        when(mMockValueAnimator.getAnimatedFraction()).thenReturn(fraction);
        mTargetAnimationListener.onAnimationUpdate(mMockValueAnimator);
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId),
                argThat(closeTo(getInterpolatedMagSpec(intermediateSpec1, newEndSpec, fraction))));
        Mockito.reset(mMockWindowManager);

        // Final value should be the new center
        Mockito.reset(mMockWindowManager);
        when(mMockValueAnimator.getAnimatedFraction()).thenReturn(1.0f);
        mTargetAnimationListener.onAnimationUpdate(mMockValueAnimator);
        verify(mMockWindowManager).setMagnificationSpec(eq(displayId),
                argThat(closeTo(newEndSpec)));
    }

    private void initMockWindowManager() {
        for (int i = 0; i < DISPLAY_COUNT; i++) {
            when(mMockWindowManager.setMagnificationCallbacks(eq(i), any())).thenReturn(true);
        }
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocationOnMock) throws Throwable {
                Object[] args = invocationOnMock.getArguments();
                Region regionArg = (Region) args[1];
                regionArg.set(INITIAL_MAGNIFICATION_REGION);
                return null;
            }
        }).when(mMockWindowManager).getMagnificationRegion(anyInt(), (Region) anyObject());
    }

    private void resetMockWindowManager() {
        Mockito.reset(mMockWindowManager);
        initMockWindowManager();
    }

    private void register(int displayId) {
        mMockValueAnimator = mock(ValueAnimator.class);
        when(mMockControllerCtx.newValueAnimator()).thenReturn(mMockValueAnimator);
        mMagnificationController.register(displayId);
        ArgumentCaptor<ValueAnimator.AnimatorUpdateListener> listenerArgumentCaptor =
                ArgumentCaptor.forClass(ValueAnimator.AnimatorUpdateListener.class);
        verify(mMockValueAnimator).addUpdateListener(listenerArgumentCaptor.capture());
        mTargetAnimationListener = listenerArgumentCaptor.getValue();
        Mockito.reset(mMockValueAnimator); // Ignore other initialization
    }

    private void zoomIn2xToMiddle(int displayId) {
        PointF startCenter = INITIAL_MAGNIFICATION_BOUNDS_CENTER;
        float scale = 2.0f;
        mMagnificationController.setScale(displayId, scale, startCenter.x, startCenter.y, false,
                SERVICE_ID_1);
        assertTrue(mMagnificationController.isMagnifying(displayId));
    }

    private MagnificationCallbacks getMagnificationCallbacks(int displayId) {
        ArgumentCaptor<MagnificationCallbacks> magnificationCallbacksCaptor =
                ArgumentCaptor.forClass(MagnificationCallbacks.class);
        verify(mMockWindowManager)
                .setMagnificationCallbacks(eq(displayId), magnificationCallbacksCaptor.capture());
        return magnificationCallbacksCaptor.getValue();
    }

    private PointF computeOffsets(Rect magnifiedBounds, PointF center, float scale) {
        return new PointF(
                magnifiedBounds.centerX() - scale * center.x,
                magnifiedBounds.centerY() - scale * center.y);
    }

    private MagnificationSpec getInterpolatedMagSpec(MagnificationSpec start, MagnificationSpec end,
            float fraction) {
        MagnificationSpec interpolatedSpec = MagnificationSpec.obtain();
        interpolatedSpec.scale = start.scale + fraction * (end.scale - start.scale);
        interpolatedSpec.offsetX = start.offsetX + fraction * (end.offsetX - start.offsetX);
        interpolatedSpec.offsetY = start.offsetY + fraction * (end.offsetY - start.offsetY);
        return interpolatedSpec;
    }

    private MagnificationSpec getMagnificationSpec(float scale, PointF offsets) {
        return getMagnificationSpec(scale, offsets.x, offsets.y);
    }

    private MagnificationSpec getMagnificationSpec(float scale, float offsetX, float offsetY) {
        MagnificationSpec spec = MagnificationSpec.obtain();
        spec.scale = scale;
        spec.offsetX = offsetX;
        spec.offsetY = offsetY;
        return spec;
    }

    private MagnificationSpec getCurrentMagnificationSpec(int displayId) {
        return getMagnificationSpec(mMagnificationController.getScale(displayId),
                mMagnificationController.getOffsetX(displayId),
                mMagnificationController.getOffsetY(displayId));
    }

    private MagSpecMatcher closeTo(MagnificationSpec spec) {
        return new MagSpecMatcher(spec, 0.01f, 0.5f);
    }

    private class MagSpecMatcher extends TypeSafeMatcher<MagnificationSpec> {
        final MagnificationSpec mMagSpec;
        final float mScaleTolerance;
        final float mOffsetTolerance;

        MagSpecMatcher(MagnificationSpec spec, float scaleTolerance, float offsetTolerance) {
            mMagSpec = spec;
            mScaleTolerance = scaleTolerance;
            mOffsetTolerance = offsetTolerance;
        }

        @Override
        protected boolean matchesSafely(MagnificationSpec magnificationSpec) {
            if (Math.abs(mMagSpec.scale - magnificationSpec.scale) > mScaleTolerance) {
                return false;
            }
            if (Math.abs(mMagSpec.offsetX - magnificationSpec.offsetX) > mOffsetTolerance) {
                return false;
            }
            if (Math.abs(mMagSpec.offsetY - magnificationSpec.offsetY) > mOffsetTolerance) {
                return false;
            }
            return true;
        }

        @Override
        public void describeTo(Description description) {
            description.appendText("Match spec: " + mMagSpec);
        }
    }
}
