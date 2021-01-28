// Code generated by MockGen. DO NOT EDIT.
// Source: tracepoint.go

// Package mock_controllers is a generated GoMock package.
package mock_controllers

import (
	gomock "github.com/golang/mock/gomock"
	uuid "github.com/satori/go.uuid"
	storepb "pixielabs.ai/pixielabs/src/vizier/services/metadata/storepb"
	reflect "reflect"
	time "time"
)

// MockTracepointStore is a mock of TracepointStore interface
type MockTracepointStore struct {
	ctrl     *gomock.Controller
	recorder *MockTracepointStoreMockRecorder
}

// MockTracepointStoreMockRecorder is the mock recorder for MockTracepointStore
type MockTracepointStoreMockRecorder struct {
	mock *MockTracepointStore
}

// NewMockTracepointStore creates a new mock instance
func NewMockTracepointStore(ctrl *gomock.Controller) *MockTracepointStore {
	mock := &MockTracepointStore{ctrl: ctrl}
	mock.recorder = &MockTracepointStoreMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use
func (m *MockTracepointStore) EXPECT() *MockTracepointStoreMockRecorder {
	return m.recorder
}

// UpsertTracepoint mocks base method
func (m *MockTracepointStore) UpsertTracepoint(arg0 uuid.UUID, arg1 *storepb.TracepointInfo) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "UpsertTracepoint", arg0, arg1)
	ret0, _ := ret[0].(error)
	return ret0
}

// UpsertTracepoint indicates an expected call of UpsertTracepoint
func (mr *MockTracepointStoreMockRecorder) UpsertTracepoint(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "UpsertTracepoint", reflect.TypeOf((*MockTracepointStore)(nil).UpsertTracepoint), arg0, arg1)
}

// GetTracepoint mocks base method
func (m *MockTracepointStore) GetTracepoint(arg0 uuid.UUID) (*storepb.TracepointInfo, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetTracepoint", arg0)
	ret0, _ := ret[0].(*storepb.TracepointInfo)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetTracepoint indicates an expected call of GetTracepoint
func (mr *MockTracepointStoreMockRecorder) GetTracepoint(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetTracepoint", reflect.TypeOf((*MockTracepointStore)(nil).GetTracepoint), arg0)
}

// GetTracepoints mocks base method
func (m *MockTracepointStore) GetTracepoints() ([]*storepb.TracepointInfo, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetTracepoints")
	ret0, _ := ret[0].([]*storepb.TracepointInfo)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetTracepoints indicates an expected call of GetTracepoints
func (mr *MockTracepointStoreMockRecorder) GetTracepoints() *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetTracepoints", reflect.TypeOf((*MockTracepointStore)(nil).GetTracepoints))
}

// UpdateTracepointState mocks base method
func (m *MockTracepointStore) UpdateTracepointState(arg0 *storepb.AgentTracepointStatus) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "UpdateTracepointState", arg0)
	ret0, _ := ret[0].(error)
	return ret0
}

// UpdateTracepointState indicates an expected call of UpdateTracepointState
func (mr *MockTracepointStoreMockRecorder) UpdateTracepointState(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "UpdateTracepointState", reflect.TypeOf((*MockTracepointStore)(nil).UpdateTracepointState), arg0)
}

// GetTracepointStates mocks base method
func (m *MockTracepointStore) GetTracepointStates(arg0 uuid.UUID) ([]*storepb.AgentTracepointStatus, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetTracepointStates", arg0)
	ret0, _ := ret[0].([]*storepb.AgentTracepointStatus)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetTracepointStates indicates an expected call of GetTracepointStates
func (mr *MockTracepointStoreMockRecorder) GetTracepointStates(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetTracepointStates", reflect.TypeOf((*MockTracepointStore)(nil).GetTracepointStates), arg0)
}

// SetTracepointWithName mocks base method
func (m *MockTracepointStore) SetTracepointWithName(arg0 string, arg1 uuid.UUID) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "SetTracepointWithName", arg0, arg1)
	ret0, _ := ret[0].(error)
	return ret0
}

// SetTracepointWithName indicates an expected call of SetTracepointWithName
func (mr *MockTracepointStoreMockRecorder) SetTracepointWithName(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "SetTracepointWithName", reflect.TypeOf((*MockTracepointStore)(nil).SetTracepointWithName), arg0, arg1)
}

// GetTracepointsWithNames mocks base method
func (m *MockTracepointStore) GetTracepointsWithNames(arg0 []string) ([]*uuid.UUID, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetTracepointsWithNames", arg0)
	ret0, _ := ret[0].([]*uuid.UUID)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetTracepointsWithNames indicates an expected call of GetTracepointsWithNames
func (mr *MockTracepointStoreMockRecorder) GetTracepointsWithNames(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetTracepointsWithNames", reflect.TypeOf((*MockTracepointStore)(nil).GetTracepointsWithNames), arg0)
}

// GetTracepointsForIDs mocks base method
func (m *MockTracepointStore) GetTracepointsForIDs(arg0 []uuid.UUID) ([]*storepb.TracepointInfo, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetTracepointsForIDs", arg0)
	ret0, _ := ret[0].([]*storepb.TracepointInfo)
	ret1, _ := ret[1].(error)
	return ret0, ret1
}

// GetTracepointsForIDs indicates an expected call of GetTracepointsForIDs
func (mr *MockTracepointStoreMockRecorder) GetTracepointsForIDs(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetTracepointsForIDs", reflect.TypeOf((*MockTracepointStore)(nil).GetTracepointsForIDs), arg0)
}

// SetTracepointTTL mocks base method
func (m *MockTracepointStore) SetTracepointTTL(arg0 uuid.UUID, arg1 time.Duration) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "SetTracepointTTL", arg0, arg1)
	ret0, _ := ret[0].(error)
	return ret0
}

// SetTracepointTTL indicates an expected call of SetTracepointTTL
func (mr *MockTracepointStoreMockRecorder) SetTracepointTTL(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "SetTracepointTTL", reflect.TypeOf((*MockTracepointStore)(nil).SetTracepointTTL), arg0, arg1)
}

// DeleteTracepointTTLs mocks base method
func (m *MockTracepointStore) DeleteTracepointTTLs(arg0 []uuid.UUID) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "DeleteTracepointTTLs", arg0)
	ret0, _ := ret[0].(error)
	return ret0
}

// DeleteTracepointTTLs indicates an expected call of DeleteTracepointTTLs
func (mr *MockTracepointStoreMockRecorder) DeleteTracepointTTLs(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "DeleteTracepointTTLs", reflect.TypeOf((*MockTracepointStore)(nil).DeleteTracepointTTLs), arg0)
}

// DeleteTracepoint mocks base method
func (m *MockTracepointStore) DeleteTracepoint(arg0 uuid.UUID) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "DeleteTracepoint", arg0)
	ret0, _ := ret[0].(error)
	return ret0
}

// DeleteTracepoint indicates an expected call of DeleteTracepoint
func (mr *MockTracepointStoreMockRecorder) DeleteTracepoint(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "DeleteTracepoint", reflect.TypeOf((*MockTracepointStore)(nil).DeleteTracepoint), arg0)
}

// DeleteTracepointsForAgent mocks base method
func (m *MockTracepointStore) DeleteTracepointsForAgent(arg0 uuid.UUID) error {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "DeleteTracepointsForAgent", arg0)
	ret0, _ := ret[0].(error)
	return ret0
}

// DeleteTracepointsForAgent indicates an expected call of DeleteTracepointsForAgent
func (mr *MockTracepointStoreMockRecorder) DeleteTracepointsForAgent(arg0 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "DeleteTracepointsForAgent", reflect.TypeOf((*MockTracepointStore)(nil).DeleteTracepointsForAgent), arg0)
}

// GetTracepointTTLs mocks base method
func (m *MockTracepointStore) GetTracepointTTLs() ([]uuid.UUID, []time.Time, error) {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "GetTracepointTTLs")
	ret0, _ := ret[0].([]uuid.UUID)
	ret1, _ := ret[1].([]time.Time)
	ret2, _ := ret[2].(error)
	return ret0, ret1, ret2
}

// GetTracepointTTLs indicates an expected call of GetTracepointTTLs
func (mr *MockTracepointStoreMockRecorder) GetTracepointTTLs() *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "GetTracepointTTLs", reflect.TypeOf((*MockTracepointStore)(nil).GetTracepointTTLs))
}
